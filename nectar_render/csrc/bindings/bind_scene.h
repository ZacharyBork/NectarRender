#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "scene/include/scene.h"
#include "scene/include/interface.h"

namespace py = pybind11;
using return_policy = py::return_value_policy;

void register_scene(py::module_& m) {
    
    auto m_scene = m.def_submodule("scene", "Scene module.");

    py::classh<Scene>(m_scene, "Scene")
        .def(py::init([](
            py::list hittables,
            py::list lights,
            std::unique_ptr<Skylight> skylight
        ) {
            std::vector<Hittable*> obj_ptrs;
            obj_ptrs.reserve(hittables.size());
            for (auto& item : hittables)
                obj_ptrs.push_back(item.cast<Hittable*>());

            std::vector<Hittable*> light_ptrs;
            light_ptrs.reserve(lights.size());
            for (auto& item : lights)
                light_ptrs.push_back(item.cast<Hittable*>());

            return Scene(obj_ptrs, light_ptrs, std::move(*skylight));
        }),
            py::arg("hittables"), 
            py::arg("lights"), 
            py::arg("skylight"),
            py::keep_alive<1, 2>(),
            py::keep_alive<1, 3>()
        );

    py::class_<SceneInterface>(m_scene, "SceneInterface")
        .def("set_material", [](
            SceneInterface& self, 
            std::unique_ptr<Material> mat
        ) {
            self.set_material(std::move(*mat));
        },
            py::arg("mat")
        )

        .def("add_object", &SceneInterface::add_object, py::arg("obj"))

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
