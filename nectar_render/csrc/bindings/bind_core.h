#pragma once

#include <pybind11/pybind11.h>

#include "core/include/core/cublas_context.h"
#include "core/include/core/utils.h"
#include "core/include/core/random.h"

void register_core(py::module_& m) {
    
    auto m_core = m.def_submodule("core", "Core module.");

    m_core.def("destroy_cublas_handle", &destroy_cublas_handle, 
        "Destroys cuBLAS handle. Registered atexit for Python module.");
    
    m_core.def("allocate_cuda_memory", &allocate_cuda_memory, "");
    m_core.def("free_cuda_memory",     &free_cuda_memory,     "");
    m_core.def("to_numpy",             &to_numpy,             "");

    // auto m_random = m.def_submodule("random", "Random module.");
    // m_random.def("seed_rng", [](unsigned int s) { RNG.seed_rng(s); });
    // m_random.def("current_seed", []() { return RNG.current_seed(); });

}

