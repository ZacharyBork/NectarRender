#pragma once

#include <pybind11/pybind11.h>
#include "interface/include/scene_interface.h"

namespace py = pybind11;
using return_policy = py::return_value_policy;

void register_interface(py::module_& m) {
    
    auto m_interface = m.def_submodule("interface", "Interface module.");

    py::class_<SceneInterface>(m_interface, "SceneInterface")
        .def("set_material", [](
            SceneInterface& self, 
            std::unique_ptr<Material> mat
        ) {
            self.set_material(std::move(*mat));
        },
            py::arg("mat")
        )
        .def("query_scene",    &SceneInterface::query_scene)
        .def("enable",         &SceneInterface::enable)
        .def("disable",        &SceneInterface::disable)
        .def("is_enabled",     &SceneInterface::is_enabled)
        .def("is_disabled",    &SceneInterface::is_disabled)
        .def("get_transform",  &SceneInterface::get_transform)
        .def("set_transform",  &SceneInterface::set_transform)
        .def("get_material",   &SceneInterface::get_material,
            return_policy::reference)
        .def("get_hit_record", &SceneInterface::get_hit_record, 
             return_policy::reference)
        
        .def("request_skylight_update", 
            &SceneInterface::request_skylight_update)
        .def("get_skylight",   &SceneInterface::get_skylight,
            return_policy::reference)
        .def("swap_skylight", [](
            SceneInterface& self,
            std::unique_ptr<Skylight> new_skylight
        ) {
            self.swap_skylight(std::move(*new_skylight));
        },
            py::arg("new_skylight")
        );
        

}

