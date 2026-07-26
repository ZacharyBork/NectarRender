#pragma once

#include <iostream>
#include <unordered_set>
#include <cuda_runtime.h>

// ############################################################################
// DEVICE OBJECT CONSTRUCTION
// ############################################################################

template<typename T, typename... Args>
T* device_build(Args... args);

// ############################################################################
// MEMORY MANAGEMENT
// ############################################################################

template<typename T>
void run_fill_memory(T* d_ptr, T value, size_t n_elements);

class CUDAMemory {
public:

    template<typename T>
    __host__ static void allocate(T*& d_ptr, size_t n_elements = 1) {    
        size_t n_bytes = n_elements * sizeof(T);
        cudaError_t err = cudaMalloc(&d_ptr, n_bytes);
        if (err != cudaSuccess) {
            std::string e = (
                "Encountered the following error while attempting to allocate "
                + std::to_string(n_bytes) + " bytes of device memory:\n"
            );
            std::cerr << e << cudaGetErrorString(err) << std::endl;
        }
    }

    template<typename T>
    __host__ static void allocate_host(T*& h_ptr, size_t n_elements = 1) {    
        size_t n_bytes = n_elements * sizeof(T);
        cudaError_t err = cudaMallocHost(&h_ptr, n_bytes);
        if (err != cudaSuccess) {
            std::string e = (
                "Encountered the following error while attempting to allocate "
                + std::to_string(n_bytes) + " bytes of host memory:\n"
            );
            std::cerr << e << cudaGetErrorString(err) << std::endl;
        }
    }
    
    template<typename T>
    __host__ static void fill(T*& d_ptr, T value, size_t n_elements) {
        size_t n_bytes = n_elements * sizeof(T);
        run_fill_memory<T>(d_ptr, value, n_bytes);
    }

    template<typename T>
    __host__ static void copy(
        T*& dest, 
        T* src, 
        size_t n_elements = 1,
        cudaMemcpyKind kind = cudaMemcpyHostToDevice
    ) {
        size_t n_bytes = n_elements * sizeof(T);
        cudaError_t err = cudaMemcpy(dest, src, n_bytes, kind);
        if (err != cudaSuccess) {
            uintptr_t dest_ptr = reinterpret_cast<uintptr_t>(dest);
            uintptr_t src_ptr = reinterpret_cast<uintptr_t>(src);
            std::string e = (
                "Encountered the following error while attempting to copy "
                + std::to_string(n_bytes) + " bytes of device memory from "
                "address " + std::to_string(src_ptr) + " to address "
                + std::to_string(dest_ptr) + ":\n"
            );
            std::cerr << e << cudaGetErrorString(err) << std::endl;
        }
    }

    template<typename T>
    __host__ static void free(T*& d_ptr) {
        if (!d_ptr) {
            uintptr_t int_ptr = reinterpret_cast<uintptr_t>(d_ptr);
            std::string w = "Warning: Encountered attempt to free invalid "
                            "pointer at address: " + std::to_string(int_ptr);
            std::cout << w << std::endl;
            return;
        }
        cudaError_t err = cudaFree(d_ptr);
        if (err != cudaSuccess) {
            uintptr_t int_ptr = reinterpret_cast<uintptr_t>(d_ptr);
            std::string e = "Encountered the following error while attempting "
                            "to free device pointer at address [" 
                            + std::to_string(int_ptr) + "]:\n";
            std::cerr << e << cudaGetErrorString(err) << std::endl;
        }
    }

    template<typename T>
    __host__ static void free_host(T*& h_ptr) {
        if (!h_ptr) {
            uintptr_t int_ptr = reinterpret_cast<uintptr_t>(h_ptr);
            std::string w = "Warning: Encountered attempt to free invalid "
                            "pointer at address: " + std::to_string(int_ptr);
            std::cout << w << std::endl;
            return;
        }
        cudaError_t err = cudaFreeHost(h_ptr);
        if (err != cudaSuccess) {
            uintptr_t int_ptr = reinterpret_cast<uintptr_t>(h_ptr);
            std::string e = "Encountered the following error while attempting "
                            "to free host pointer at address [" 
                            + std::to_string(int_ptr) + "]:\n";
            std::cerr << e << cudaGetErrorString(err) << std::endl;
        }
    }

};

// ############################################################################
// CUDA PROCESS UTILITIES
// ############################################################################

struct ProcessIndex {
    int x, y, z;
    __device__ ProcessIndex(int x, int y, int z) : x(x), y(y), z(z) { }
};

__device__ ProcessIndex get_process_index();



