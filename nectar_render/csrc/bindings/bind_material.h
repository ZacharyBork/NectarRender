#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "material/include/material/material.h"

namespace py = pybind11;

void register_material(py::module_& m) {
    
    auto m_material = m.def_submodule("material", "Material module.");
    auto m_texture = m_material.def_submodule("texture", "Texture submodule.");

    /* TEXTURE CLASS */

    py::enum_<TextureType>(m_texture, "TextureType")
        .value("CONSTANT", TextureType::CONSTANT)
        .value("IMAGE",      TextureType::IMAGE)
        .def("__repr__", [](const TextureType& type) {
            switch (type) {
                case TextureType::CONSTANT: return "TextureType.CONSTANT";
                case TextureType::IMAGE:    return "TextureType.IMAGE";
                default: return "TextureType.UNKNOWN";
            }
        });

    py::class_<Texture, std::shared_ptr<Texture>>(m_texture, "Texture")
        .def_static("from_color",
            py::overload_cast<const Color&>(&Texture::from_color),
            py::arg("color"))
        .def_static("from_color",
            py::overload_cast<float, float, float>(&Texture::from_color),
            py::arg("r"), py::arg("g"), py::arg("b"))
        .def_static("from_image", &Texture::from_image, py::arg("filepath"))

        .def_readonly("type",           &Texture::type)
        .def_readonly("filepath",       &Texture::filepath)
        .def_readonly("constant_color", &Texture::constant_color)
        .def_readonly("C",              &Texture::C)
        .def_readonly("H",              &Texture::H)
        .def_readonly("W",              &Texture::W)

        .def("__repr__", [](const Texture& tex) {
            std::string output = "Texture(type = ";
            switch (tex.type) {
                case TextureType::CONSTANT: output += "CONSTANT"; break;
                case TextureType::IMAGE:    output += "IMAGE";    break;
                default: output += "UNKNOWN"; break;
            }
            return output;
        });

    /* MATERIAL CLASS */

    py::enum_<MaterialType>(m_material, "MaterialType")
        .value("Lambertian", MaterialType::Lambertian)
        .value("PBR",        MaterialType::PBR)
        .value("Dielectric", MaterialType::Dielectric)
        .value("Emissive",   MaterialType::Emissive)
        .value("Isotropic",  MaterialType::Isotropic)
        .def("__repr__", [](const MaterialType& type) {
            std::string output = "MaterialType.";
            switch (type) {
                case MaterialType::Lambertian: output += "Lambertian"; break;
                case MaterialType::PBR:        output += "PBR";        break;
                case MaterialType::Dielectric: output += "Dielectric"; break;
                case MaterialType::Emissive:   output += "Emissive";   break;
                case MaterialType::Isotropic:  output += "Isotropic";  break;
                default: output += "UNKNOWN"; break;
            }
            return output;
        });

    py::class_<MaterialCore>(m_material, "MaterialCore");
    py::class_<Lambertian, MaterialCore>  (m_material, "Lambertian");
    py::class_<PBR,        MaterialCore>  (m_material, "PBR");
    py::class_<Dielectric, MaterialCore>  (m_material, "Dielectric");
    py::class_<Emissive,   MaterialCore>  (m_material, "Emissive");
    py::class_<Isotropic,  MaterialCore>  (m_material, "Isotropic");

    py::classh<Material>(m_material, "Material")
        .def(py::init<>())
        .def("material_type",       &Material::material_type)
        .def("get_tracked_texture", &Material::get_tracked_texture,
            return_policy::reference)

        .def("texture_count", &Material::texture_count)
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

        /* LAMBERTIAN */

        .def_static("lambertian",
            py::overload_cast<const Color&>(&Material::lambertian),
            py::arg("color") = Color::white())
        .def_static("lambertian",
            py::overload_cast<std::shared_ptr<Texture>>(&Material::lambertian),
            py::arg("texture") = Texture::from_color(Color::white()))

        /* PBR */

        .def_static("pbr", &Material::pbr,
            py::arg("albedo")    = Texture::from_color(Color::white()),
            py::arg("roughness") = Texture::from_color(Color(0.8f)),
            py::arg("metallic")  = Texture::from_color(Color(0.0f)),
            py::arg("emission")  = Texture::from_color(Color::black()),
            py::arg("normal")    = Texture::from_color(Color(0.5f, 0.5f, 1.0f))
        )

        /* DIELECTRIC */

        .def_static("dielectric", &Material::dielectric, py::arg("ior") = 1.5f)

        /* EMISSIVE */

        .def_static("emissive",
            py::overload_cast<const Color&, float>(&Material::emissive),
            py::arg("albedo") = Color::white(),
            py::arg("brightness") = 35.0f
        )
        .def_static("emissive",
            py::overload_cast<std::shared_ptr<Texture>, float>(
                &Material::emissive
            ),
            py::arg("texture") = Texture::from_color(Color::white()),
            py::arg("brightness") = 35.0f
        )

        /* ISOTROPIC */

        .def_static("isotropic",
            py::overload_cast<const Color&>(&Material::isotropic),
            py::arg("albedo") = Color::white())
        .def_static("isotropic",
            py::overload_cast<std::shared_ptr<Texture>>(&Material::isotropic),
            py::arg("texture") = Texture::from_color(Color::white()));


}

