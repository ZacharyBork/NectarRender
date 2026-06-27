#pragma once

#include <cuda_runtime.h>
#include <stdint.h>

#include "core/include/core/vector.h"

// ############################################################################
// MEMORY MANAGEMENT
// ############################################################################

uintptr_t allocate_cuda_memory(size_t n_elements, float fill_value);
void free_cuda_memory(uintptr_t device_ptr);
void cuda_synchronize();

// ############################################################################
// CUDA PROCESS UTILS
// ############################################################################

struct ProcessIndex {
    int x, y, z;
    __device__ ProcessIndex(int x, int y, int z) : x(x), y(y), z(z) { }
};

__device__ ProcessIndex get_process_index();

// ############################################################################
// COLOR UTILITIES
// ############################################################################

struct ColorIndex { 
    int r, g, b; 

    /* CONSTRUCTION */

    __host__ __device__ ColorIndex(int r, int g, int b) 
        : r(r), g(g), b(b) { }

    __device__ static ColorIndex from_process(
        unsigned int C,
        unsigned int H,
        unsigned int W
    ) {
        ProcessIndex p_idx = get_process_index();
        return ColorIndex(
            p_idx.z * (C * H * W) + 0 * (H * W) + p_idx.y * W + p_idx.x, 
            p_idx.z * (C * H * W) + 1 * (H * W) + p_idx.y * W + p_idx.x, 
            p_idx.z * (C * H * W) + 2 * (H * W) + p_idx.y * W + p_idx.x
        );
    }

    /* UTILITIES */

    __host__ __device__ Color get_color(float* data_ptr) const {
        return Color(data_ptr[r], data_ptr[g], data_ptr[b]);
    }

    __host__ __device__ void set_color(float* data_ptr, Color c) const {
        data_ptr[r] = c.r();
        data_ptr[g] = c.g();
        data_ptr[b] = c.b();
    }
};

// ############################################################################
// VALUE CONVERSION
// ############################################################################

__host__ __device__ inline float deg2rad(float degrees) {
    return degrees * 0.01745329; 
}

__host__ __device__ inline Vector3 deg2rad(const Vector3& degrees) {
    return Vector3(
        deg2rad(degrees.x()), 
        deg2rad(degrees.y()), 
        deg2rad(degrees.z())
    );
}

__host__ __device__ inline float rad2deg(float radians) {
    return radians * 57.29578;
}

__host__ __device__ inline Vector3 rad2deg(const Vector3& radians) {
    return Vector3(
        rad2deg(radians.x()), 
        rad2deg(radians.y()), 
        rad2deg(radians.z())
    );
}

