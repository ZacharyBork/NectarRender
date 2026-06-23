#pragma once

#include <pybind11/pybind11.h>
#include "host/include/host/utils.h"

namespace py = pybind11;

void register_host(py::module_& m) {
    
    auto m_host = m.def_submodule("host", "Host module.");

    m_host.def("to_numpy", &to_numpy, "");

}

