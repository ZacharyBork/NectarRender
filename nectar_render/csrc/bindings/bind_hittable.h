#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hittable/include/hittable/hittable.h"
#include "hittable/include/hittable/primitives.h"
#include "hittable/include/hittable/shapes.h"

namespace py = pybind11;

void register_hittable(py::module_& m) {
    
    auto m_hittable = m.def_submodule("hittable", "Hittable module.");

    py::class_<Hittable>(m_hittable, "Hittable");

    py::class_<Sphere, Hittable>(m_hittable, "Sphere")
        .def(py::init([](
            const Vector3 center, 
            float radius, 
            const Material& material
        ) {
            return Sphere(center, radius, material);
        }),
            py::arg("center")   = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("radius")   = 1.0f,
            py::arg("material") = Lambertian(Color(0.8f, 0.8f, 0.8f))
        )
        .def("set_motion_vector", &Hittable::set_motion_vector);

    py::class_<Quad, Hittable>(m_hittable, "Quad")
        .def(py::init([](
            const Vector3 position, 
            const Vector3 rotation, 
            const Vector3 scale, 
            const Material& material
        ) {
            return Quad(position, rotation, scale, material);
        }),
            py::arg("position") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("rotation") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("scale")    = Vector3(1.0f, 1.0f, 1.0f),
            py::arg("material") = Lambertian(Color(0.8f, 0.8f, 0.8f))
        )
        .def("set_motion_vector", &Hittable::set_motion_vector);

}

