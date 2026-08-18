#pragma once

#include <pybind11/pybind11.h>

#include "host/include/host/utils.h"
#include "host/include/devtools/scratchpad.h"

namespace py = pybind11;

void register_host(py::module_& m) {

    /* SUBMODULES */

    auto m_host = m.def_submodule("host", "Host module.");
    auto m_memory = m_host.def_submodule("memory", "Memory utils submodule.");
    auto m_devtools = m_host.def_submodule("devtools", "Devtools submodule.");

    /* MEMORY */

    py::class_<CUDAMemInfo>(m_memory, "CUDAMemInfo")
        .def(py::init<>())
        .def_readonly("used",  &CUDAMemInfo::used)
        .def_readonly("free",  &CUDAMemInfo::free)
        .def_readonly("total", &CUDAMemInfo::total);

    m_memory.def("get_cuda_meminfo",      &get_cuda_meminfo);
    m_memory.def("get_cuda_memory_used",  &get_cuda_memory_used);
    m_memory.def("get_cuda_memory_free",  &get_cuda_memory_free);
    m_memory.def("get_cuda_memory_total", &get_cuda_memory_total);

    /* DEVTOOLS */

    py::class_<ScratchPad>(m_devtools, "ScratchPad")
        .def(py::init<>())    
        .def("run", &ScratchPad::run);


}

