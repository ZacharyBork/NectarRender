#pragma once

#include <pybind11/pybind11.h>

#include "host/include/devtools/scratchpad.h"

namespace py = pybind11;

void register_host(py::module_& m) {

    auto m_host = m.def_submodule("host", "Host module.");
    auto m_devtools = m_host.def_submodule("devtools", "Devtools submodule.");

    py::class_<ScratchPad>(m_devtools, "ScratchPad")
        .def(py::init<>())    
        .def("run", &ScratchPad::run);


}

