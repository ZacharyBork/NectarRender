#pragma once

#include <stdint.h>
#include <unordered_map>
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
    size_t H, W;
    Camera camera;
    RenderLayers aovs, sample_aovs;

    uint32_t num_samples = 10;
    uint32_t max_depth   = 8;

    std::function<void(uint32_t)> on_frame_finished;
    
    RenderEngine(
        const Camera& camera,
        uint32_t samples   = 10,
        uint32_t ray_depth = 8,
        uint32_t seed      = 54321
    ) : camera(camera), 
        aovs(RenderLayers(camera.resolution)),
        sample_aovs(RenderLayers(&aovs)),
        num_samples(samples),
        max_depth(ray_depth),
        seed(seed)
    { }

    void sample(Scene& scene, SampleMode mode = SampleMode::ACCUMULATE) {
        sample_idx++;

        trace(camera, scene, sample_aovs, max_depth, seed, sample_idx);
        if (mode == SampleMode::ACCUMULATE) 
            aovs.accumulate(sample_aovs, sample_idx);
        else aovs.combine(sample_aovs);
        sample_aovs.clear();

        cuda_synchronize();
    }

    void render(Scene& scene, SampleMode mode = SampleMode::ACCUMULATE) {
        aovs.pin_buffer(LayerType::BEAUTY);
        
        for (int s = 0; s < num_samples; s++) {
            sample(scene, mode);
            on_frame_finished(sample_idx);
        }
    }

    RenderLayers* layers() { return &aovs; }

private:

    Hittable** d_objects = nullptr;
    uint32_t seed;
    uint32_t sample_idx = 0;

};
