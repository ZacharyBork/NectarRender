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

    uint32_t sample_idx = 0u; // Index of current sample
    uint32_t n_samples = 0u;  // Total number of samples run

    /* SAMPLE UTILITIES */
    
    __host__ __device__ void reset_sample_counter(const uint32_t count = 0u) { 
        n_samples = count;
    }

    __host__ void set_sample_index(const uint32_t idx) {
        sample_idx = idx;
        n_samples++;
    }
    
    __host__ __device__ void reset_sample_indices(
        const uint32_t idx = 0u
    ) { 
        sample_idx = idx;
    }

    __host__ __device__ void reset_sample_parameters(
        const uint32_t count = 0u,
        const uint32_t idx = 0u
    ) { 
        n_samples = count;
        sample_idx = idx;
    }

};

void trace(
    TraceConfig   cfg, 
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs
);

