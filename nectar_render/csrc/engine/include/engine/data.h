#pragma once

#include <vector>
#include <stdint.h>
#include <cuda_runtime.h>

#include "core/include/core/utils.h"
#include "core/include/core/vector.h"

class DataObject {
public:
    uintptr_t device_ptr;
    
    size_t C, H, W; // Channels, Height, Width
    size_t n_elements;

    DataObject() 
        : device_ptr(0), C(0), H(0), W(0), n_elements(0) {}

    DataObject(
        size_t n_channels, 
        size_t h, 
        size_t w
    ) : C(n_channels), H(h), W(w) {
        n_elements = n_channels * H * W;
        device_ptr = allocate_cuda_memory(n_elements, 0.0f);
    }

    DataObject(std::vector<size_t> shape) {
        C = shape[0]; H = shape[1]; W = shape[2];
        n_elements = C * H * W;
        device_ptr = allocate_cuda_memory(n_elements, 0.0f);
    }

    DataObject(
        uintptr_t d_ptr,
        size_t    n_channels, 
        size_t    h, 
        size_t    w
    ) : device_ptr(d_ptr), C(n_channels), H(h), W(w) {
        n_elements = n_channels * H * W;
    }

    __host__ __device__ float* data_ptr() { 
        return reinterpret_cast<float*>(device_ptr); 
    }

    __device__ void set_color(int x, int y, int z, Color color) {
        int r = z * (C * H * W) + 0 * (H * W) + y * W + x;
        int g = z * (C * H * W) + 1 * (H * W) + y * W + x;
        int b = z * (C * H * W) + 2 * (H * W) + y * W + x;

        float* d_ptr = data_ptr();
        d_ptr[r] = color.x();
        d_ptr[g] = color.y();
        d_ptr[b] = color.z();
    }

};

