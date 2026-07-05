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

    py::class_<ImageTexture, Texture>(m_texture, "ImageTexture")
        .def(py::init([](
            uintptr_t    host_ptr, 
            const size_t channels,
            const size_t height,
            const size_t width
        ) {
            return ImageTexture(host_ptr, channels, height, width);
        }),
            py::arg("host_ptr"),
            py::arg("channels"),
            py::arg("height"),
            py::arg("width")
        )
        .def("n_bytes", &ImageTexture::n_bytes);

    py::class_<NoiseTexture, Texture>(m_texture, "NoiseTexture")
        .def(py::init<>()) 
        .def(py::init([](
            float scale,
            uint32_t seed
        ) {
            return NoiseTexture(scale, seed);
        }),
            py::arg("scale"),
            py::arg("seed") = 42u
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

    py::class_<Emissive, Material>(m_material, "Emissive")
        .def(py::init([](
            const Color& albedo,
            const float brightness
        ) {
            return Emissive(albedo, brightness);
        }),
            py::arg("albedo")     = Color(0.8f, 0.8f, 0.8f),
            py::arg("brightness") = 35.0f
        )
        .def(py::init([](
            const Texture& texture,
            const float brightness
        ) {
            return Emissive(texture, brightness);
        }),
            py::arg("texture"),
            py::arg("brightness") = 35.0f
        );

    py::class_<Isotropic, Material>(m_material, "Isotropic")
        .def(py::init([](const Color& albedo) {
            return Isotropic(albedo);
        }),
            py::arg("albedo") = Color(0.8f, 0.8f, 0.8f)
        )
        .def(py::init([](const Texture& texture) {
            return Isotropic(texture);
        }));


}

