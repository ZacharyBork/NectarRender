#pragma once

#include <cuda_runtime.h>

#include "engine/include/engine/data.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/light.h"

struct TraceConfig {
    size_t H = (size_t)512;
    size_t W = (size_t)512;

    DeviceCamera* camera = nullptr;
    SceneGraph*   scene  = nullptr;
    AOVs*         aovs   = nullptr; 

    uint32_t max_depth = 8u;
    uint32_t seed      = 54321u;
    uint32_t frame     = 0u;
    
    __host__ void increment() { frame++; }

    __host__ void update_scene_graph(SceneGraph* graph) {
        if (!scene) scene = graph;
    }

};

void trace(TraceConfig cfg);

