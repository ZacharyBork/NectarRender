#pragma once

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

    RenderEngine(
        Camera&  camera,
        uint32_t ray_depth = 8u,
        uint32_t seed      = 54321u
    ) : cam(camera),
        aovs(RenderLayers(cam.resolution)),
        sample_aovs(RenderLayers(&aovs))
    { 
        config.H = (size_t)cam.resolution[0];
        config.W = (size_t)cam.resolution[1];
        
        cam.__construct();
        config.camera = cam.device_camera();
        config.aovs   = sample_aovs.aovs();

        config.max_depth = ray_depth;
        config.seed = seed;
    }

    void sample(
        Scene& scene, 
        uint32_t s_x = 0u,
        uint32_t s_y = 0u,
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        config.update_scene_graph(scene.graph);
        config.set_sample_index(s_x, s_y);
        config.increment();

        trace(config);

        if (mode == SampleMode::ACCUMULATE) 
            aovs.accumulate(sample_aovs, config.frame);
        else aovs.combine(sample_aovs);
        sample_aovs.clear();

        cuda_synchronize();
    }

    void render(
        Scene& scene, 
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        aovs.pin_buffer(LayerType::BEAUTY);
        
        uint32_t n_samples_stratified = cam.sqrt_n_samples();
        for (uint32_t s_y = 0; s_y < n_samples_stratified; s_y++) {
            for (uint32_t s_x = 0; s_x < n_samples_stratified; s_x++) {
                sample(scene, s_x, s_y, mode);
                on_frame_finished(config.frame);
            }
        }
    }

    RenderLayers* layers() { return &aovs; }

private:

    TraceConfig config;
    RenderLayers aovs, sample_aovs;

};
