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
    HittablesList device_scene;
    std::optional<CameraParams> cam_params;
    std::optional<RenderLayers> render_layers;

    unsigned int num_samples = 10;
    unsigned int max_depth = 8;

    std::function<void(int)> on_frame_finished;
    
    void initialize(
        const CameraParams& camera_params,
        unsigned int samples,
        unsigned int ray_depth,
        unsigned int seed
    );

    void render(const std::vector<Hittable*>& scene);
    RenderLayers get_render_layers();

private:

    Hittable** d_objects = nullptr;
    unsigned int random_seed;
    unsigned int frame_idx = 0;
    bool initialized = false;

    void build_scene(const std::vector<Hittable*>& host_scene);
};

static RenderEngine engine;

