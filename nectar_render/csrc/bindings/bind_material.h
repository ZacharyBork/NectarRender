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
        .def(py::init([](const Color albedo) {
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
        )
        .def("__repr__", [](ConstantTexture& t) {
            return "ConstantTexture(" 
                 + std::to_string(t.color()[0]) + ", "
                 + std::to_string(t.color()[1]) + ", "
                 + std::to_string(t.color()[2]) + ")";
        });

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

    /* MATERIAL CLASS */

    py::enum_<MaterialType>(m_material, "MaterialType")
        .value("Lambertian", MaterialType::Lambertian)
        .value("PBR",        MaterialType::PBR)
        .value("Dielectric", MaterialType::Dielectric)
        .value("Emissive",   MaterialType::Emissive)
        .value("Isotropic",  MaterialType::Isotropic)
        .def("__repr__", [](const MaterialType& type) {
            switch (type) {
                case MaterialType::Lambertian: 
                    return "MaterialType.Lambertian";
                    break;
                case MaterialType::PBR:        
                    return "MaterialType.PBR";
                    break;
                case MaterialType::Dielectric: 
                    return "MaterialType.Dielectric";
                    break;
                case MaterialType::Emissive:   
                    return "MaterialType.Emissive";
                    break;
                case MaterialType::Isotropic:  
                    return "MaterialType.Isotropic";
                    break;
                default: return "MaterialType.UNKNOWN";
            }
        });

    py::class_<MaterialCore>(m_material, "MaterialCore");
    py::class_<Lambertian>  (m_material, "Lambertian");
    py::class_<PBR>         (m_material, "PBR");
    py::class_<Dielectric>  (m_material, "Dielectric");
    py::class_<Emissive>    (m_material, "Emissive");
    py::class_<Isotropic>   (m_material, "Isotropic");

    py::class_<Material>(m_material, "Material")
        .def(py::init<>())
        .def("__repr__", [](const Material& mat) {
            std::string output = "Material(type = ";
            switch (mat.material_type()) {
                case MaterialType::Lambertian: output += "Lambertian"; break;
                case MaterialType::PBR:        output += "PBR";        break;
                case MaterialType::Dielectric: output += "Dielectric"; break;
                case MaterialType::Emissive:   output += "Emissive";   break;
                case MaterialType::Isotropic:  output += "Isotropic";  break;
                default: output += "UNKNOWN";
            }
            return output + ")";
        })

        .def_static("lambertian", ([](const Color& albedo) {
            return Material::lambertian(albedo);
        }),
            py::arg("albedo") = Color(0.8f, 0.8f, 0.8f)
        )
        .def_static("lambertian", ([](const Texture& texture) {
            return Material::lambertian(texture);
        }),
            py::arg("texture") = ConstantTexture(Color(0.8f, 0.8f, 0.8f))
        )


        .def_static("pbr", ([](
            const Texture& albedo,
            const Texture& roughness,
            const Texture& metallic,
            const Texture& emission,
            const Texture& normal
        ) {
            return Material::pbr(
                albedo, roughness, metallic, emission, normal
            );
        }),
            py::arg("albedo")    = ConstantTexture(Color::white()),
            py::arg("roughness") = ConstantTexture(Color(0.8f)),
            py::arg("metallic")  = ConstantTexture(Color(0.0f)),
            py::arg("emission")  = ConstantTexture(Color::black()),
            py::arg("normal")    = ConstantTexture(Color(0.5f, 0.5f, 1.0f))
        )


        .def_static("dielectric", ([](float ior) {
            return Material::dielectric(ior);
        }),
            py::arg("ior") = 1.5f
        )


        .def_static("emissive", ([](
            const Color& albedo, 
            const float brightness
        ) {
            return Material::emissive(albedo, brightness);
        }),
            py::arg("albedo")     = Color::white(),
            py::arg("brightness") = 35.0f
        )
        .def_static("emissive", ([](
            const Texture& texture, 
            const float brightness
        ) {
            return Material::emissive(texture, brightness);
        }),
            py::arg("texture")    = ConstantTexture(Color::white()),
            py::arg("brightness") = 35.0f
        )


        .def_static("isotropic", ([](const Color& albedo) {
            return Material::isotropic(albedo);
        }),
            py::arg("albedo") = Color::white()
        )
        .def_static("isotropic", ([](const Texture& texture) {
            return Material::isotropic(texture);
        }),
            py::arg("texture") = ConstantTexture(Color::white())
        );

}

