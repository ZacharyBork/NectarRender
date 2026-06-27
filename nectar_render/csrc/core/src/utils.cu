#include "core/include/core/utils.h"

// ############################################################################
// MEMORY MANAGEMENT
// ############################################################################

__global__ void fill_cuda_memory_kernel(
    float* dst, 
    size_t n, 
    float  fill_value
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    dst[idx] = fill_value;
}

uintptr_t allocate_cuda_memory(
    size_t n_elements, 
    float  fill_value
) {
    size_t nbytes = n_elements * sizeof(float);
    float* d_ptr;
    cudaMalloc(&d_ptr, nbytes);

    int block = 256;
    int grid = (n_elements + block - 1) / block;
    fill_cuda_memory_kernel<<<grid, block>>>(d_ptr, n_elements, fill_value);

    return reinterpret_cast<uintptr_t>(d_ptr);
}

void free_cuda_memory(uintptr_t device_ptr) {
    cudaFree(reinterpret_cast<void*>(device_ptr));
}

void cuda_synchronize() { cudaDeviceSynchronize(); }

// ############################################################################
// CUDA PROCESS UTILS
// ############################################################################

__device__ ProcessIndex get_process_index() {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    return ProcessIndex(x, y, blockIdx.z);
}

// ############################################################################
// COLOR UTILITIES
// ############################################################################

__device__ ColorIndex get_color_index(
    ProcessIndex p_idx, 
    size_t C, 
    size_t H, 
    size_t W
) {
    int r = p_idx.z * (C * H * W) + 0 * (H * W) + p_idx.y * W + p_idx.x;
    int g = p_idx.z * (C * H * W) + 1 * (H * W) + p_idx.y * W + p_idx.x;
    int b = p_idx.z * (C * H * W) + 2 * (H * W) + p_idx.y * W + p_idx.x;

    return ColorIndex(r, g, b);
}

__device__ ColorIndex get_color_index(
    int x, int y, int z,
    size_t C, 
    size_t H, 
    size_t W
) {
    int r = z * (C * H * W) + 0 * (H * W) + y * W + x;
    int g = z * (C * H * W) + 1 * (H * W) + y * W + x;
    int b = z * (C * H * W) + 2 * (H * W) + y * W + x;
    return ColorIndex(r, g, b);
}

