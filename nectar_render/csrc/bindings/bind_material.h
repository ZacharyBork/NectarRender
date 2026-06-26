#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "material/include/material/material.h"

namespace py = pybind11;

void register_material(py::module_& m) {
    
    auto m_material = m.def_submodule("material", "Material module.");
    auto m_texture = m_material.def_submodule("texture", "Texture submodule.");

    /* TEXTURE CLASSES */

    py::class_<Texture>(m_texture, "Texture");

    py::class_<ConstantTexture, Texture>(m_texture, "ConstantTexture")
        .def(py::init([](const Color& albedo) {
            return ConstantTexture(albedo);
        }),
            py::arg("albedo") = Color(0.8f, 0.8f, 0.8f)
        )
        .def(py::init([](float r, float g, float b) {
            return ConstantTexture(r, g, b);
        }),
            py::arg("r") = 0.8f,
            py::arg("g") = 0.8f,
            py::arg("b") = 0.8f
        );

    py::class_<CheckerTexture, Texture>(m_texture, "CheckerTexture")
        .def(py::init([](
            const Color& color1,
            const Color& color2,
            float scale
        ) {
            return CheckerTexture(color1, color2, scale);
        }),
            py::arg("color1") = Color(0.0f, 0.0f, 0.0f),
            py::arg("color1") = Color(1.0f, 1.0f, 1.0f),
            py::arg("scale")  = 1.0f
        );

    /* MATERIAL CLASSES */

    py::class_<Material>(m_material, "Material");

    py::class_<Lambertian, Material>(m_material, "Lambertian")
        .def(py::init([](const Color& albedo) {
            return Lambertian(albedo);
        }),
            py::arg("albedo") = Color(0.8f, 0.8f, 0.8f)
        )
        .def(py::init([](const Texture& texture) {
            return Lambertian(texture);
        }));

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

