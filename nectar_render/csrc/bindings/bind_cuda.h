#pragma once

#include <pybind11/pybind11.h>

#include "core/include/core.h"
#include "core/include/core/cublas_context.h"

namespace py = pybind11;

void register_cuda(py::module_& m) {
    
    auto m_cuda = m.def_submodule("cuda", "CUDA utility module.");

    m_cuda.def("destroy_cublas_handle", []() { 
        py::gil_scoped_acquire acquire;
        destroy_cublas_handle(); 
    }, "Destroys cuBLAS handle. Registered atexit for Python module.");

    m_cuda.def("cudaDeviceSynchronize", []() { 
        py::gil_scoped_acquire acquire;
        cudaError_t err = cudaDeviceSynchronize(); 
        if (err != cudaSuccess) {
            std::string e = (
                "Encountered the following error while attempting to "
                "synchronize device:\n"
            );
            std::cerr << e << cudaGetErrorString(err) << std::endl;
        }
    });

}

