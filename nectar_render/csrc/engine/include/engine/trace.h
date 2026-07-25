#pragma once

#include <cuda_runtime.h>

#include "data/include/data.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/light.h"

struct TraceConfig {
    size_t H = (size_t)512;
    size_t W = (size_t)512;

    uint32_t seed = 54321u;

};

void trace(
    TraceConfig   cfg, 
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs,
    uint32_t      sample_idx,
    uint32_t      ray_depth
);

void hit_test_ray(
    float u, 
    float v, 
    SceneGraph* scene, 
    DeviceCamera* cam,
    HitRecord* rec
);

