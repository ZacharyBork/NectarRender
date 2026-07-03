#pragma once

#include <optional>
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
        
        cam._construct();
        config.camera = cam.device_camera();
        config.aovs   = sample_aovs.aovs();

        config.max_depth = ray_depth;
        config.seed = seed;
    }

    void sample(
        Scene& scene, 
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        config.update_scene_graph(scene.graph);
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
        
        for (int s = 0; s < cam.n_samples; s++) {
            sample(scene, mode);
            on_frame_finished(config.frame);
        }
    }

    RenderLayers* layers() { return &aovs; }

private:

    Camera cam;
    TraceConfig config;
    RenderLayers aovs, sample_aovs;

};
