#pragma once

#include <cuda_runtime.h>

#include "data/include/data.h"
#include "engine/include/engine/camera.h"
#include "scene/include/scene.h"
#include "light/include/skylight.h"

struct TraceConfig {
    uint32_t sample_idx;
    uint32_t ray_depth;
    uint32_t seed;

    // Russian roulette path termination
    bool     do_rr_termination  = true;
    uint32_t rr_start_bounce    = 3u;
    float    rr_survival_chance = 0.95;
};

void trace_full(
    Camera&     cam,
    SceneGraph* scene,
    AOVs*       aovs,
    TraceConfig cfg
);

void trace_viewport(
    Camera&     cam,
    SceneGraph* scene,
    AOVs*       aovs,
    bool        show_axis_grid,
    uint32_t    seed
);

void hit_test_ray(
    float u, 
    float v, 
    SceneGraph* scene, 
    DeviceCamera* cam,
    HitRecord* rec
);

