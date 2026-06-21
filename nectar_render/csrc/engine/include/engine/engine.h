#pragma once

#include <stdint.h>
#include <cuda_runtime.h>
#include <vector>

#include "engine/include/engine/data.h"

class RenderEngine {
public:
    DataObject color;
    std::vector<size_t> output_shape;
    
    void initialize(
        std::vector<size_t> _output_shape, 
        unsigned int random_seed
    );

    uintptr_t render();

private:

    unsigned int random_seed;
    unsigned int frame_idx = 0;
    bool initialized = false;
};

static RenderEngine engine;

