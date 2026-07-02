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

    size_t H, W;
    uint32_t max_depth = 8;

    Camera camera;
    RenderLayers aovs, sample_aovs;

    std::function<void(uint32_t)> on_frame_finished;

    std::optional<TraceConfig> config;
    const bool config_initialized() const { return config.has_value(); }
    
    RenderEngine(
        const Camera& camera,
        uint32_t ray_depth = 8u,
        uint32_t seed      = 54321u
    ) : camera(camera), 
        aovs(RenderLayers(camera.resolution)),
        sample_aovs(RenderLayers(&aovs)),
        max_depth(ray_depth),
        seed(seed)
    { }

    void sample(
        Scene& scene, 
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        if (!config_initialized()) update_config(scene);
        config->increment();

        trace(config.value());
        if (mode == SampleMode::ACCUMULATE) 
            aovs.accumulate(sample_aovs, config->frame);
        else aovs.combine(sample_aovs);
        sample_aovs.clear();

        cuda_synchronize();
    }

    void render(
        Scene& scene, 
        uint32_t num_samples = 100u,
        SampleMode mode = SampleMode::ACCUMULATE
    ) {
        aovs.pin_buffer(LayerType::BEAUTY);
        
        for (int s = 0; s < num_samples; s++) {
            sample(scene, mode);
            on_frame_finished(config->frame);
        }
    }

    RenderLayers* layers() { return &aovs; }

private:

    Hittable** d_objects = nullptr;
    uint32_t   seed;
    uint32_t   sample_idx = 0;

    void update_config(Scene& scene) {
        config.emplace(
            TraceConfig{ 
                camera.device_camera(), scene.graph(), 
                sample_aovs.aovs(), max_depth, seed 
            }
        );
    }

};
