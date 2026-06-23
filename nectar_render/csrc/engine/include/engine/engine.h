#pragma once

#include <stdint.h>
#include <cuda_runtime.h>
#include <vector>
#include <optional>

#include "core/include/core/utils.h"
#include "engine/include/engine/data.h"
#include "hittable/include/hittable/hittable.h"

class RenderEngine {
public:
    std::vector<size_t> output_shape;
    std::optional<RenderLayers> render_layers;
    HittablesList scene;

    unsigned int samples_per_pixel = 10;
    unsigned int max_depth = 8;
    
    void initialize(
        std::vector<size_t> _output_shape, 
        unsigned int random_seed
    );

    void build_scene(const std::vector<std::shared_ptr<Hittable>>& host_scene);

    uintptr_t render();

private:

    Hittable** d_objects = nullptr;
    unsigned int random_seed;
    unsigned int frame_idx = 0;
    bool initialized = false;
};

static RenderEngine engine;

