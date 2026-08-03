#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hittable/include/hittable.h"

namespace py = pybind11;

void register_hittable(py::module_& m) {
    
    auto m_hittable = m.def_submodule("hittable", "Hittable module.");

    py::class_<HitRecord>(m_hittable, "HitRecord")
        .def(py::init<>())
        .def("d_object_ptr",          &HitRecord::d_object_ptr)
        .def_readonly("hit_object",   &HitRecord::hit_object)
        .def_readonly("object_index", &HitRecord::object_index)
        .def_readonly("p",            &HitRecord::p)
        .def_readonly("n",            &HitRecord::n)
        .def_readonly("tangent",      &HitRecord::tangent)
        .def_readonly("uv",           &HitRecord::uv)
        .def_readonly("t",            &HitRecord::t)
        .def_readonly("front_face",   &HitRecord::front_face)
        .def_readonly("mat",          &HitRecord::mat);

    py::class_<Quad>          (m_hittable, "Quad");
    py::class_<Sphere>        (m_hittable, "Sphere");
    py::class_<Cube>          (m_hittable, "Cube");
    py::class_<Mesh>          (m_hittable, "Mesh");
    py::class_<ObjectLight>   (m_hittable, "ObjectLight");
    py::class_<ConstantMedium>(m_hittable, "ConstantMedium");
    
    py::class_<Hittable>(m_hittable, "Hittable")
        .def(py::init<>())
        

        .def_static("quad", [](
            Vector3 position, 
            Vector3 rotation, 
            Vector3 scale,
            std::unique_ptr<Material> material
        ) {
            return Hittable::quad(
                position, rotation, scale, std::move(*material)
            );
        },
            py::arg("position") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("rotation") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("scale")    = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("material") = make_default_material()
        )


        .def_static("cube", [](
            Vector3 position, 
            Vector3 rotation, 
            Vector3 scale,
            std::unique_ptr<Material> material
        ) {
            return Hittable::cube(
                position, rotation, scale, std::move(*material)
            );
        },
            py::arg("position") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("rotation") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("scale")    = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("material") = make_default_material()
        )


        .def_static("sphere", [](
            Vector3 position, 
            float radius,
            std::unique_ptr<Material> material
        ) {
            return Hittable::sphere(
                position, radius, std::move(*material)
            );
        },
            py::arg("position") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("radius")   = 1.0f,
            py::arg("material") = make_default_material()
        )

        
        .def_static("mesh", [](
            std::string& path,
            Vector3 position, 
            Vector3 rotation, 
            Vector3 scale,
            std::unique_ptr<Material> material
        ) {
            return Hittable::mesh(
                path, position, rotation, scale, std::move(*material)
            );
        },
            py::arg("path"),
            py::arg("position") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("rotation") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("scale")    = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("material") = make_default_material()
        )


        .def_static("constant_medium", [](
            Hittable& bound_obj,
            float density,
            std::shared_ptr<Texture> texture
        ){ 
            return Hittable::constant_medium(bound_obj, density, texture);
        },
            py::arg("bound_obj"),
            py::arg("density") = 1.0f,
            py::arg("texture") = Texture::from_color(Color(1.0f, 1.0f, 1.0f))
        )

        .def_static("constant_medium", [](
            Hittable& bound_obj,
            float density,
            const Color& albedo
        ){ 
            return Hittable::constant_medium(bound_obj, density, albedo);
        },
            py::arg("bound_obj"),
            py::arg("density") = 1.0f,
            py::arg("albedo")  = Color(1.0f, 1.0f, 1.0f)
        )

        .def_static("object_light", [](
            Hittable& bound_obj,
            float brightness,
            std::shared_ptr<Texture> texture
        ){ 
            return Hittable::object_light(bound_obj, brightness, texture);
        },
            py::arg("bound_obj"),
            py::arg("brightness") = 1.0f,
            py::arg("texture") = Texture::from_color(Color(1.0f, 1.0f, 1.0f))
        )

        .def_static("object_light", [](
            Hittable& bound_obj,
            float brightness,
            const Color& albedo
        ){ 
            return Hittable::object_light(bound_obj, brightness, albedo);
        },
            py::arg("bound_obj"),
            py::arg("brightness") = 1.0f,
            py::arg("albedo") = Color(1.0f, 1.0f, 1.0f)
        );

}

