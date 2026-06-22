#pragma once

#include <stdint.h>
#include <cuda_runtime.h>
#include <vector>

#include "engine/include/engine/data.h"
#include "hittable/include/hittable/hittable.h"

class RenderEngine {
public:
    DataObject color;
    HittablesList scene;
    std::vector<size_t> output_shape;
    
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

