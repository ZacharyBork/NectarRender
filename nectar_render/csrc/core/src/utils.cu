#include "core/include/core/utils.h"

// ############################################################################
// MEMORY MANAGEMENT
// ############################################################################

template<typename T>
__global__ void fill_memory_kernel(T* d_ptr, T value, size_t n_elements) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_elements) return;
    d_ptr[idx] = value;
}

template<typename T>
void run_fill_memory(T* d_ptr, T value, size_t n_elements) {
    fill_memory_kernel<<<1, 1>>>(d_ptr, value, n_elements);
}

template void run_fill_memory<float>(float*, float, size_t);
template void run_fill_memory<int>(int*, int, size_t);
template void run_fill_memory<size_t>(size_t*, size_t, size_t);
template void run_fill_memory<uint8_t>(uint8_t*, uint8_t, size_t);
template void run_fill_memory<uint32_t>(uint32_t*, uint32_t, size_t);

template void run_fill_memory<Vector2>(Vector2*, Vector2, size_t);
template void run_fill_memory<Vector3>(Vector3*, Vector3, size_t);
template void run_fill_memory<Color>(Color*, Color, size_t);

// ############################################################################
// CUDA PROCESS UTILS
// ############################################################################

__device__ ProcessIndex get_process_index() {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    return ProcessIndex(x, y, blockIdx.z);
}

