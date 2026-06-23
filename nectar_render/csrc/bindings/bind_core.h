#pragma once

#include <pybind11/pybind11.h>

#include "core/include/core/cublas_context.h"
#include "core/include/core/utils.h"
#include "core/include/core/random.h"

namespace py = pybind11;

void register_core(py::module_& m) {
    
    auto m_core = m.def_submodule("core", "Core module.");

    m_core.def("destroy_cublas_handle", &destroy_cublas_handle, 
        "Destroys cuBLAS handle. Registered atexit for Python module.");
    
    m_core.def("allocate_cuda_memory", &allocate_cuda_memory, "");
    m_core.def("free_cuda_memory",     &free_cuda_memory,     "");

}

