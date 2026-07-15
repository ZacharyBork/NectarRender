#pragma once

#include <atomic>
#include <optional>
#include <stdint.h>
#include <cuda_runtime.h>
#include <pybind11/functional.h>

#include "core/include/core.h"
#include "host/include/host/utils.h"
#include "engine/include/engine/data.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/trace.h"
#include "hittable/include/hittable/hittable.h"
#include "interface/include/object_interface.h"

enum class SampleMode{ ACCUMULATE, COMBINE };
enum class EngineState{ IDLE, RENDERING, PAUSED, CANCELLED, RESETTING };

class RenderEngine {
public:

    std::function<void()>         on_render_started  = hook_no_op<>;
    std::function<void(uint32_t)> on_frame_finished  = hook_no_op<uint32_t>;
    std::function<void()>         on_render_finished = hook_no_op<>;
    std::function<void()>         on_paused          = hook_no_op<>;
    std::function<void()>         on_canceled        = hook_no_op<>;
    std::function<void()>         on_reset           = hook_no_op<>;

    /* CONSTRUCTION */

    ~RenderEngine() {
        if (current_scene) current_scene->teardown();
    }

    RenderEngine(
        const Camera& camera,
        uint32_t ray_depth = 8u,
        uint32_t seed      = 54321u
    ) : cam(camera),
        ray_depth(ray_depth),
        seed(seed),
        aovs(RenderLayers(cam.resolution)),
        sample_aovs(RenderLayers(&aovs))
    { 
        reset();
        aovs.pin_buffer(LayerType::BEAUTY);
    }

    /* PROPERTY ACCESS */

    Camera*       camera() { return &cam; }
    RenderLayers* layers() { return &aovs; }

    Scene* scene() { 
        if (!current_scene)
            throw std::runtime_error(
                "RenderEngine::scene() called on engine without emplaced "
                "scene instance.");
        return current_scene; 
    }

    /* ENGINE STATE */

    EngineState get_state() const {
        return state.load(std::memory_order_relaxed);
    }

    bool is_idle()      const { return get_state()==EngineState::IDLE;      }
    bool is_rendering() const { return get_state()==EngineState::RENDERING; }
    bool is_paused()    const { return get_state()==EngineState::PAUSED;    }
    bool is_cancelled() const { return get_state()==EngineState::CANCELLED; }
    bool is_resetting() const { return get_state()==EngineState::RESETTING; }
    
    /* SAMPLING / RENDERING */

    void sample(
        Scene&     input_scene, 
        uint32_t   sample_index = 1u,
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        trace(
            config, cam.device_camera(), input_scene.graph, 
            sample_aovs.aovs(), sample_index
        );

        sample_aovs.replace_invalid_values();
        if (mode == SampleMode::ACCUMULATE) 
            aovs.accumulate(sample_aovs, sample_index);
        else aovs.combine(sample_aovs);
        sample_aovs.clear();

        if (interface.is_enabled())
            interface.build_selection_mask(
                config.H, config.W, cam.device_camera(), 
                input_scene.graph, aovs.beauty
            );

        cudaDeviceSynchronize();
    }

    void render(
        Scene&     input_scene, 
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        sample_mode = mode;
        current_scene = &input_scene;
        set_state(EngineState::RENDERING);
        with_gil_scoped_acquire(on_render_started);
        
        uint32_t n_samples = cam.n_samples + 1u;

        for (uint32_t idx = sample_idx; idx < n_samples; idx++) {
            if (is_cancelled()) { cancel_render(); return; }
            if (is_paused())    { pause_render();  return; }
            if (is_resetting()) { 
                cudaDeviceSynchronize();
                reset();
                with_gil_scoped_acquire(on_reset);

                idx = 1u;
                set_state(EngineState::RENDERING);
            }

            sample(*current_scene, idx, mode);
            with_gil_scoped_acquire(on_frame_finished, idx);
            sample_idx++;
        }

        cudaDeviceSynchronize();
        with_gil_scoped_acquire(on_render_finished);
        set_state(EngineState::IDLE);
    }

    /* UTILITIES */

    void reset() {
        cam.__construct(seed);
        
        config.H = (size_t)cam.resolution[0];
        config.W = (size_t)cam.resolution[1];
        config.max_depth = ray_depth;
        config.seed = seed;

        sample_idx = 1u;

        aovs.clear();

        if (bvh_build_pending.load(std::memory_order_relaxed)) {
            bvh_build_pending.store(false, std::memory_order_relaxed);
            scene()->build();
        }
    }

    const uint32_t n_samples() const { return cam.n_samples; }
    void set_n_samples(uint32_t n) {
        cudaDeviceSynchronize();
        cam.n_samples = n;
        reset();
    }

    const uint32_t max_depth() const { return ray_depth; }
    void set_max_depth(uint32_t value) {
        cudaDeviceSynchronize();
        ray_depth = value;
        reset();
    }

    ObjectInterface& screen_space_ray(float u, float v) {
        HitRecord* d_rec;

        cudaMalloc(&d_rec, sizeof(HitRecord));
        hit_test_ray(
            u, v, current_scene->graph, cam.device_camera(), d_rec
        );

        HitRecord rec;
        cudaMemcpy(&rec, d_rec, sizeof(HitRecord), cudaMemcpyDeviceToHost);
        cudaFree(d_rec);

        request_reset();
        cudaDeviceSynchronize();

        interface = ObjectInterface(current_scene, rec);

        return interface;  
    }

    void request_pause()  { set_state(EngineState::PAUSED);    }
    void request_cancel() { set_state(EngineState::CANCELLED); }
    void request_reset(const bool rebuild_bvh = false)  { 
        bvh_build_pending.store(rebuild_bvh, std::memory_order_relaxed);
        if (is_rendering()) set_state(EngineState::RESETTING);
        else reset(); 
    }

private:

    std::atomic<EngineState> state { EngineState::IDLE };
    std::atomic<bool> bvh_build_pending { false };

    Camera       cam;
    SampleMode   sample_mode = SampleMode::ACCUMULATE;
    TraceConfig  config;
    RenderLayers aovs, sample_aovs;
    uint32_t     ray_depth, seed;

    uint32_t sample_idx = 1u;

    Scene* current_scene = nullptr;
    ObjectInterface interface;

    __host__ void set_state(EngineState s) {
        state.store(s, std::memory_order_relaxed);
    }

    __host__ void cancel_render() {
        cudaDeviceSynchronize();
        with_gil_scoped_acquire(on_canceled);
        set_state(EngineState::IDLE);
    }

    __host__ void pause_render() {
        cudaDeviceSynchronize();
        with_gil_scoped_acquire(on_paused);
    }

};
