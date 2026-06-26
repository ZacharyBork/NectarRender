#pragma once

#include <cuda_runtime.h>
#include <stdint.h>

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

    __host__ __device__ ColorIndex(int r, int g, int b) 
        : r(r), g(g), b(b) { }
};

struct ColorValues { 
    float r, g, b; 

    __host__ __device__ ColorValues(float r, float g, float b) 
        : r(r), g(g), b(b) { }

    __host__ __device__ ColorValues(float* data_ptr, ColorIndex index) {
        r = data_ptr[index.r];
        g = data_ptr[index.g];
        b = data_ptr[index.b];
    }
};

__device__ ColorIndex get_color_index(
    ProcessIndex p_idx, 
    size_t C, 
    size_t H, 
    size_t W
);

__device__ ColorIndex get_color_index(
    int x, int y, int z,
    size_t C, 
    size_t H, 
    size_t W
);

// ############################################################################
// VALUE CONVERSION
// ############################################################################

__host__ __device__ float deg2rad(float degrees);
__host__ __device__ float rad2deg(float radians);
