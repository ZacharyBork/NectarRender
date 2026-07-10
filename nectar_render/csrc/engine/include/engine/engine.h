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

enum class SampleMode{ ACCUMULATE, COMBINE };
enum class EngineState{ IDLE, RENDERING, PAUSED, CANCELLED, REFRESHING };

class RenderEngine {
public:

    std::function<void()>         on_render_started  = hook_no_op<>;
    std::function<void(uint32_t)> on_frame_finished  = hook_no_op<uint32_t>;
    std::function<void()>         on_render_finished = hook_no_op<>;
    std::function<void()>         on_paused          = hook_no_op<>;
    std::function<void()>         on_canceled        = hook_no_op<>;
    std::function<void()>         on_refreshed       = hook_no_op<>;

    /* CONSTRUCTION */

    RenderEngine(
        Camera&  camera,
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
        if (!current_scene.has_value())
            throw std::runtime_error(
                "RenderEngine::scene() called on engine without emplaced "
                "scene instance.");
        return &current_scene.value(); 
    }

    /* ENGINE STATE */

    EngineState get_state() const {
        return state.load(std::memory_order_relaxed);
    }

    bool is_idle()       const { return get_state()==EngineState::IDLE;       }
    bool is_rendering()  const { return get_state()==EngineState::RENDERING;  }
    bool is_paused()     const { return get_state()==EngineState::PAUSED;     }
    bool is_cancelled()  const { return get_state()==EngineState::CANCELLED;  }
    bool is_refreshing() const { return get_state()==EngineState::REFRESHING; }
    
    /* SAMPLING / RENDERING */

    void sample(
        Scene& scene, 
        uint32_t s_x = 0u,
        uint32_t s_y = 0u,
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        config.set_sample_index(s_x, s_y);

        trace(config, cam.device_camera(), scene.graph, sample_aovs.aovs());
        sample_aovs.replace_invalid_values();

        if (mode == SampleMode::ACCUMULATE) 
            aovs.accumulate(sample_aovs, config.n_samples);
        else aovs.combine(sample_aovs);
        sample_aovs.clear();

        cuda_synchronize();
    }

    void render(
        Scene& scene, 
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        current_scene.emplace(scene);
        set_state(EngineState::RENDERING);
        with_gil_scoped_acquire(on_render_started);

        uint32_t n_samples = cam.sqrt_n_samples();
        
        for (uint32_t s_y = 0; s_y < n_samples - y_samples; s_y++) {            
            for (uint32_t s_x = 0; s_x < n_samples - x_samples; s_x++) {
                if (is_cancelled())  { cancel_render();        return; }
                if (is_paused())     { pause_render(s_x, s_y); return; }
                if (is_refreshing()) { break; }

                sample(scene, s_x, s_y, mode);
                with_gil_scoped_acquire(on_frame_finished, config.n_samples);
            }
        }

        cuda_synchronize();
        if (is_refreshing()) {
            with_gil_scoped_acquire(on_refreshed);
            render(scene, mode);
        } else {
            set_state(EngineState::IDLE);
            with_gil_scoped_acquire(on_render_finished);
        }
    }

    /* UTILITIES */

    void request_pause()   { set_state(EngineState::PAUSED);    }
    void request_cancel()  { set_state(EngineState::CANCELLED); }
    void request_refresh() { set_state(EngineState::REFRESHING); }

    void reset() {
        set_state(EngineState::IDLE);

        cam.__construct();
        config.H = (size_t)cam.resolution[0];
        config.W = (size_t)cam.resolution[1];
        config.max_depth = ray_depth;
        config.seed = seed;

        config.s_x = config.s_y = 0u;
        config.n_samples = 0u;

        x_samples = 0u; y_samples = 0u;
    }

private:

    std::atomic<EngineState> state { EngineState::IDLE };

    Camera       cam;
    TraceConfig  config;
    RenderLayers aovs, sample_aovs;
    uint32_t     ray_depth, seed;

    uint32_t x_samples = 0u;
    uint32_t y_samples = 0u;

    std::optional<Scene> current_scene;

    __host__ void set_state(EngineState s) {
        state.store(s, std::memory_order_relaxed);
    }

    __host__ void cancel_render() {
        cuda_synchronize();
        with_gil_scoped_acquire(on_canceled);
    }

    __host__ void pause_render(uint32_t s_x, uint32_t s_y) {
        cuda_synchronize();
        x_samples = s_x; y_samples = s_y;
        with_gil_scoped_acquire(on_paused);
    }

};
