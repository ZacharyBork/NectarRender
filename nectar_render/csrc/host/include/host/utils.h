#pragma once

#include <cuda_runtime.h>
#include <pybind11/functional.h>

namespace py = pybind11;

// ############################################################################
// GIL CONTROLS
// ############################################################################

template<typename F, typename... Args>
void with_gil_scoped_acquire(F& func, Args... args) {
    {
        py::gil_scoped_acquire acquire;
        std::forward<F>(func)(std::forward<Args>(args)...);
    }
}

template<typename F, typename... Args>
void with_gil_scoped_release(F& func, Args... args) {
    {
        py::gil_scoped_release release;
        std::forward<F>(func)(std::forward<Args>(args)...);
    }
}

// ############################################################################
// PROFILING & STATISTICS
// ############################################################################

struct CUDAMemInfo { size_t used, free, total; };

inline CUDAMemInfo get_cuda_meminfo() {
    size_t free, total;
    cudaMemGetInfo(&free, &total);
    return CUDAMemInfo{ total - free, free, total };
}

inline size_t get_cuda_memory_used()  { return get_cuda_meminfo().used;  }
inline size_t get_cuda_memory_free()  { return get_cuda_meminfo().free;  }
inline size_t get_cuda_memory_total() { return get_cuda_meminfo().total; }

