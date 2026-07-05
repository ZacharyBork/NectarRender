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

    uint32_t s_x = 0u;       // X index of current sample
    uint32_t s_y = 0u;       // Y index of current sample
    uint32_t n_samples = 0u; // Total number of samples run

    /* SAMPLE UTILITIES */
    
    __host__ __device__ void reset_sample_counter(const uint32_t count = 0u) { 
        n_samples = count;
    }

    __host__ void set_sample_index(const uint32_t x, const uint32_t y) {
        s_x = x; s_y = y;
        n_samples++;
    }
    
    __host__ __device__ void reset_sample_indices(
        const uint32_t x = 0u,
        const uint32_t y = 0u
    ) { 
        s_x = x; s_y = y;
    }

    __host__ __device__ void reset_sample_parameters(
        const uint32_t count = 0u,
        const uint32_t x = 0u,
        const uint32_t y = 0u
    ) { 
        n_samples = count;
        s_x = x; s_y = y;
    }

};

void trace(
    TraceConfig   cfg, 
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs
);

