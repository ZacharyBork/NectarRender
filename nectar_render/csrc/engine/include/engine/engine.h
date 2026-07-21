#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <pybind11/functional.h>

#include "core/include/core.h"
#include "host/include/host/utils.h"
#include "hittable/include/hittable/hittable.h"
#include "interface/include/scene_interface.h"

#include "data.h"
#include "scene.h"
#include "camera.h"
#include "trace.h"

enum class SampleMode{ ACCUMULATE, COMBINE };
enum class EngineState{ IDLE, RENDERING };

class RenderEngine {
public:

    std::function<void()>         on_render_started  = hook_no_op<>;
    std::function<void(uint32_t)> on_frame_finished  = hook_no_op<uint32_t>;
    std::function<void()>         on_render_finished = hook_no_op<>;
    std::function<void()>         on_stopped         = hook_no_op<>;
    std::function<void()>         on_reset           = hook_no_op<>;

    /* CONSTRUCTION */

    ~RenderEngine() { current_scene.teardown(); }

    RenderEngine(
        const Camera& camera,
        uint32_t ray_depth = 8u,
        uint32_t seed      = 54321u
    ) : cam(camera),
        ray_depth(ray_depth),
        seed(seed),
        aovs(RenderLayers(cam.resolution())),
        sample_aovs(RenderLayers(&aovs))
    { 
        reset();
        transfer_stream.link(aovs.get_layer(LayerType::BEAUTY));
        transfer_stream.start();
    }

    /* PROPERTY ACCESS */

    Scene*          scene()  { return &current_scene; }
    Camera*         camera() { return &cam; }
    RenderLayers*   layers() { return &aovs; }
    TransferStream* stream() { return &transfer_stream; }

    /* ENGINE STATE */

    EngineState get_state() const {
        return state.load(std::memory_order_relaxed);
    }

    bool is_idle()      const { return get_state()==EngineState::IDLE; }
    bool is_rendering() const { 
        return get_state()==EngineState::RENDERING
            && !stop_render.load(std::memory_order_relaxed);
    }
    
    void request_stop() { stop_render.store(true, std::memory_order_relaxed); }

    /* FUNCTION QUEUE */

    void queue_function(
        std::function<void()> func, 
        bool rebuild_bvh = false, 
        bool immediate = true
    ) {
        { 
            std::lock_guard<std::mutex> lock(function_queue_mutex); 
            function_queue.push_back(func); 
        }
        if (rebuild_bvh)
            bvh_build_pending.store(true, std::memory_order_relaxed);
    }

    /* SAMPLING / RENDERING */

    void set_scene(Scene input_scene) { current_scene = input_scene; }
    void set_sample_mode(SampleMode mode) { sample_mode = mode; }

    void render() {
        set_state(EngineState::RENDERING);
        with_gil_scoped_acquire(on_render_started);

        uint32_t n_samples = cam.n_samples() + 1u;

        for (uint32_t idx = sample_idx; idx < n_samples; idx++) {
            if (stop_render.load(std::memory_order_relaxed)) {
                cudaDeviceSynchronize();
                with_gil_scoped_acquire(on_stopped);
                stop_render.store(false, std::memory_order_relaxed);
                set_state(EngineState::IDLE);
                return;
            }

            sample(idx);
            with_gil_scoped_acquire(on_frame_finished, idx);
            sample_idx++;
        }

        cudaDeviceSynchronize();
        with_gil_scoped_acquire(on_render_finished);
        set_state(EngineState::IDLE);
    }

    /* RESET */

    __host__ void reset() {
        cudaDeviceSynchronize();

        process_function_queue();

        cam.__construct(seed);
        
        config.H = (size_t)cam.resolution()[0];
        config.W = (size_t)cam.resolution()[1];
        config.max_depth = ray_depth;
        config.seed = seed;

        sample_idx = 1u;

        aovs.clear();

        if (bvh_build_pending.load(std::memory_order_relaxed)) {
            bvh_build_pending.store(false, std::memory_order_relaxed);
            scene()->build();
        }

        with_gil_scoped_acquire(on_reset);

    }

    /* UTILITIES */

    const uint32_t n_samples() const { return cam.n_samples(); }
    void set_n_samples(uint32_t n) {
        cudaDeviceSynchronize();
        cam.parameters_()->samples_per_pixel = n;
        reset();
    }

    const uint32_t max_depth() const { return ray_depth; }
    void set_max_depth(uint32_t value) {
        cudaDeviceSynchronize();
        ray_depth = value;
        reset();
    }

    /* SCENE INTERFACE UTILS */

    SceneInterface& get_scene_interface() { return scene_interface; }

    void screen_space_ray(float u, float v) {
        std::cout << "Before: " << scene_interface.is_enabled() << std::endl;

        if (scene_interface.is_enabled()) scene_interface.disable();

        std::cout << "After: " << scene_interface.is_enabled() << "\n" << std::endl;

        HitRecord* d_rec;
        cudaMalloc(&d_rec, sizeof(HitRecord));
        hit_test_ray(u, v, current_scene.graph, cam.device_camera(), d_rec);

        HitRecord rec;
        cudaMemcpy(&rec, d_rec, sizeof(HitRecord), cudaMemcpyDeviceToHost);
        cudaFree(d_rec);

        scene_interface = SceneInterface(&current_scene, rec);
    }

private:

    std::atomic<bool> stop_render { false };
    std::atomic<bool> bvh_build_pending { false };
    std::atomic<EngineState> state { EngineState::IDLE };

    std::mutex function_queue_mutex;
    std::vector<std::function<void()>> function_queue{};

    Camera       cam;
    SampleMode   sample_mode = SampleMode::ACCUMULATE;
    TraceConfig  config;
    RenderLayers aovs, sample_aovs;
    uint32_t     ray_depth, seed;

    uint32_t sample_idx = 1u;

    Scene current_scene;
    SceneInterface scene_interface;
    TransferStream transfer_stream;

    __host__ void set_state(EngineState s) {
        state.store(s, std::memory_order_relaxed);
    }

    __host__ void process_function_queue() {
        std::vector<std::function<void()>> to_run;
        { 
            std::lock_guard<std::mutex> lock(function_queue_mutex); 
            to_run.swap(function_queue); 
        }
        if (to_run.empty()) return;

        {
            py::gil_scoped_acquire acquire;
            for (auto& func : to_run) func();
            to_run.clear();
        }
        cudaDeviceSynchronize();
    }

    void sample(uint32_t sample_index = 1u) {
        trace(
            config, cam.device_camera(), current_scene.graph, 
            sample_aovs.aovs(), sample_index
        );

        sample_aovs.replace_invalid_values();
        if (sample_mode == SampleMode::ACCUMULATE) 
            aovs.accumulate(sample_aovs, sample_index);
        else aovs.combine(sample_aovs);
        sample_aovs.clear();

        if (scene_interface.is_enabled())
            scene_interface.build_selection_mask(
                config.H, config.W, cam.device_camera(), 
                current_scene.graph, aovs.beauty
            );

        cudaDeviceSynchronize();
    }

};


