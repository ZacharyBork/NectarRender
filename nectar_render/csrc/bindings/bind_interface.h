#pragma once

#include <pybind11/pybind11.h>
#include "interface/include/scene_interface.h"

namespace py = pybind11;
using return_policy = py::return_value_policy;

void register_interface(py::module_& m) {
    
    auto m_interface = m.def_submodule("interface", "Interface module.");

    py::class_<SceneInterface>(m_interface, "SceneInterface")
        .def("update_material", [](
            SceneInterface& self, 
            const Material& mat
        ) {
            self.update_material(mat);
        },
            py::arg("mat")
        )
        .def("disable",        &SceneInterface::disable)
        .def("is_enabled",     &SceneInterface::is_enabled)
        .def("get_transform",  &SceneInterface::get_transform)
        .def("set_transform",  &SceneInterface::set_transform)
        .def("get_material",   &SceneInterface::get_material)
        .def("get_hit_record", &SceneInterface::get_hit_record, 
             return_policy::reference);
        

}

