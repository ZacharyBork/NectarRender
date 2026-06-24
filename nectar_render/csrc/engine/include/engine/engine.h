#pragma once

#include <stdint.h>
#include <cuda_runtime.h>
#include <vector>
#include <optional>
#include <pybind11/functional.h>

#include "core/include/core/utils.h"
#include "engine/include/engine/data.h"
#include "engine/include/engine/camera.h"
#include "hittable/include/hittable/hittable.h"

class RenderEngine {
public:
    size_t H, W;
    HittablesList scene;
    std::optional<CameraParams> cam_params;
    std::optional<RenderLayers> render_layers;

    unsigned int num_samples = 10;
    unsigned int max_depth = 8;

    std::function<void(int, int, float)> on_sample;
    
    void initialize(
        const CameraParams& camera_params,
        unsigned int samples,
        unsigned int ray_depth,
        unsigned int seed
    );

    void build_scene(const std::vector<Hittable*>& host_scene);
    uintptr_t render();

private:

    Hittable** d_objects = nullptr;
    unsigned int random_seed;
    unsigned int frame_idx = 0;
    bool initialized = false;
};

static RenderEngine engine;

