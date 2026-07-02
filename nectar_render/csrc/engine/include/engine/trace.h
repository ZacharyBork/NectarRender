#pragma once

#include <cuda_runtime.h>

#include "engine/include/engine/data.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/light.h"

struct TraceConfig {
    DeviceCamera* camera;
    SceneGraph*   scene;
    AOVs          aovs; 
    uint32_t      max_depth;
    uint32_t      seed;
    uint32_t      frame = 0u;

    size_t H = aovs.H;
    size_t W = aovs.W;
    
    __host__ void increment() { frame++; }

};

void trace(TraceConfig cfg);

