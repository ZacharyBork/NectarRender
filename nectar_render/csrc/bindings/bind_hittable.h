#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hittable/include/hittable/hittable.h"

namespace py = pybind11;

void register_hittable(py::module_& m) {
    
    auto m_hittable = m.def_submodule("hittable", "Hittable module.");

    py::class_<Hittable>(m_hittable, "Hittable");

    py::class_<Sphere, Hittable>(m_hittable, "Sphere")
        .def(py::init([](
            const Vector3 center, 
            float radius, 
            const Lambertian& mat
        ) {
            return Sphere(center, radius, mat);
        }),
            py::arg("center")   = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("radius")   = 1.0f,
            py::arg("material") = Lambertian(Color(0.8f, 0.8f, 0.8f))
        );

}

