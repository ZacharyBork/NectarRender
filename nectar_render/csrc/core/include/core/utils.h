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

// ############################################################################
// VALUE INTERPRETATION
// ############################################################################

__host__ __device__ inline uint32_t float_as_uint(float f) {
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

