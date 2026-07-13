#pragma once

#include <cuda_runtime.h>

#include "engine/include/engine/data.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/light.h"

struct TraceConfig {
    size_t H = (size_t)512;
    size_t W = (size_t)512;

    uint32_t max_depth = 8u;
    uint32_t seed      = 54321u;

};

void trace(
    TraceConfig   cfg, 
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs,
    uint32_t      sample_idx
);

void hit_test_ray(
    float u, 
    float v, 
    SceneGraph* scene, 
    DeviceCamera* cam,
    HitRecord* rec
);

