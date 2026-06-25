#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "material/include/material/material.h"

namespace py = pybind11;

void register_material(py::module_& m) {
    
    auto m_material = m.def_submodule("material", "Material module.");

    py::class_<Material>(m_material, "Material");

    py::class_<Lambertian, Material>(m_material, "Lambertian")
        .def(py::init([](const Color& albedo) {
            return Lambertian(albedo);
        }),
            py::arg("albedo") = Color(0.8f, 0.8f, 0.8f)
        );

    py::class_<Metal, Material>(m_material, "Metal")
        .def(py::init([](const Color& albedo, float fuzz) {
            return Metal(albedo, fuzz);
        }),
            py::arg("albedo") = Color(0.8f, 0.8f, 0.8f),
            py::arg("fuzz")   = 0.0f
        );

    py::class_<Dielectric, Material>(m_material, "Dielectric")
        .def(py::init([](float ior) {
            return Dielectric(ior);
        }),
            py::arg("ior") = 1.5f
        );

}

