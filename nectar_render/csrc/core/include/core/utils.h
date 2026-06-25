#pragma once

#include <cuda_runtime.h>
#include <stdint.h>

/* MEMORY MANAGEMENT */

uintptr_t allocate_cuda_memory(size_t n_elements, float fill_value);
void free_cuda_memory(uintptr_t device_ptr);
void cuda_synchronize();

/* CUDA PROCESS UTILS */

struct ProcessIndex {
    int x, y, z;
    __device__ ProcessIndex(int x, int y, int z) : x(x), y(y), z(z) { }
};

__device__ ProcessIndex get_process_index();

/* VALUE CONVERSION */

__host__ __device__ float deg2rad(float degrees);
__host__ __device__ float rad2deg(float radians);
