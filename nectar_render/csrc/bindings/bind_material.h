#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "material/include/material/material.h"

namespace py = pybind11;

void register_material(py::module_& m) {
    
    auto m_material = m.def_submodule("material", "Material module.");

    py::class_<Lambertian>(m_material, "Lambertian")
        .def(py::init([](Color albedo) {
            return Lambertian(albedo);
        }),
            py::arg("albedo") = Color(0.8f, 0.8f, 0.8f)
        );

}

