#pragma once

#include <pybind11/pybind11.h>

#include "engine/include/engine/engine.h"

namespace py = pybind11;

void register_engine(py::module_& m) {
    
    auto m_engine = m.def_submodule("engine", "Engine module.");

    m_engine.def("initialize", [](
            std::vector<size_t> _output_shape,
            unsigned int random_seed = 54321
        ) { 
            engine.initialize(_output_shape, random_seed); 
        }
    );

    m_engine.def("render", []() { return engine.render(); });

}

