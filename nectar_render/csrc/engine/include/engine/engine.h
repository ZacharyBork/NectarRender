#pragma once

#include <atomic>
#include <stdint.h>
#include <cuda_runtime.h>
#include <pybind11/functional.h>

#include "core/include/core.h"
#include "engine/include/engine/data.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/trace.h"
#include "hittable/include/hittable/hittable.h"

enum class SampleMode{ ACCUMULATE, COMBINE };

class RenderEngine {
public:

    Camera cam;
    std::function<void(uint32_t)> on_frame_finished;
    std::function<void()>         on_render_finished;
    std::function<void()>         on_canceled;

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
        cam.__construct();
        config.H = (size_t)cam.resolution[0];
        config.W = (size_t)cam.resolution[1];
        config.max_depth = ray_depth;
        config.seed = seed;

        aovs.pin_buffer(LayerType::BEAUTY);
    }

    /* PROPERTY ACCESS */

    RenderLayers* layers() { return &aovs; }
    
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
        cancel_requested.store(false, std::memory_order_relaxed);
        rendering.store(true, std::memory_order_relaxed);

        uint32_t n_samples_stratified = cam.sqrt_n_samples();
        
        for (uint32_t s_y = 0; s_y < n_samples_stratified; s_y++) {
            for (uint32_t s_x = 0; s_x < n_samples_stratified; s_x++) {
                if (is_cancelled()) { 
                    cuda_synchronize();
                    rendering.store(false, std::memory_order_relaxed);
                    { 
                        py::gil_scoped_acquire acquire;
                        on_canceled(); 
                    }
                    return; 
                } 
                
                sample(scene, s_x, s_y, mode);
                
                {
                    py::gil_scoped_acquire acquire;
                    on_frame_finished(config.n_samples);
                }
            }
        }

        cuda_synchronize();
        rendering.store(false, std::memory_order_relaxed);
        on_render_finished();
    }

    /* UTILITIES */

    void request_cancel() { 
        cancel_requested.store(true, std::memory_order_relaxed); 
    }

    bool is_cancelled() const { 
        return cancel_requested.load(std::memory_order_relaxed); 
    }

    bool is_rendering() const { 
        return rendering.load(std::memory_order_relaxed); 
    }

    void reset() {
        cam.__construct();
        config.H = (size_t)cam.resolution[0];
        config.W = (size_t)cam.resolution[1];
        config.max_depth = ray_depth;
        config.seed = seed;

        config.s_x = config.s_y = 0u;
        config.n_samples = 0u;
    }

private:

    TraceConfig  config;
    RenderLayers aovs, sample_aovs;
    uint32_t     ray_depth, seed;

    std::atomic<bool> rendering{ false };
    std::atomic<bool> cancel_requested{ false };

};
