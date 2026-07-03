#pragma once

#include <pybind11/pybind11.h>

#include "core/include/core.h"
#include "core/include/core/cublas_context.h"

namespace py = pybind11;

void register_core(py::module_& m) {
    
    auto m_core = m.def_submodule("core", "Core module.");

    /* UTILS */

    auto m_utils = m_core.def_submodule("utils", "Core utility submodule.");

    m_utils.def("destroy_cublas_handle", &destroy_cublas_handle, 
        "Destroys cuBLAS handle. Registered atexit for Python module.");
    
    m_utils.def("allocate_cuda_memory", &allocate_cuda_memory, "");
    m_utils.def("free_cuda_memory",     &free_cuda_memory,     "");
    m_utils.def("cuda_synchronize",     &cuda_synchronize,     "");

    /* RNG */

    auto m_random = m_core.def_submodule("random", "Core RNG submodule.");
    py::class_<Generator>(m_random, "Generator");

    /* VECTORS */

    auto m_vec = m_core.def_submodule("vector", "Vector module.");

    py::class_<Vector<Vector2>>(m_vec, "VectorBase2");
    py::class_<Vec2Core<Vector2>, Vector<Vector2>>(m_vec, "Vec2Core2");
    py::class_<Vector2, Vec2Core<Vector2>>(m_vec, "Vector2")
        .def(py::init([](float e) {
            return Vector2(e);
        }),
            py::arg("e") = 0.0f
        )
        .def(py::init([](float x, float y) {
            return Vector2(x, y);
        }),
            py::arg("x") = 0.0f,
            py::arg("y") = 0.0f
        )
        .def("__repr__", [](const Vector2& v) {
            return "Vector2(" + std::to_string(v.x()) + ", " 
                              + std::to_string(v.y()) + ")";
        })
        .def("x", &Vector2::x)
        .def("y", &Vector2::y)
        .def("u", &Vector2::u)
        .def("v", &Vector2::v);

    py::class_<Vector<Vector3>>(m_vec, "VectorBase3");
    py::class_<Vec3Core<Vector3>, Vector<Vector3>>(m_vec, "Vec3Core3");
    py::class_<Vector3, Vec3Core<Vector3>>(m_vec, "Vector3")
        .def(py::init([](float e) {
            return Vector3(e);
        }),
            py::arg("e") = 0.0f
        )
        .def(py::init([](float x, float y, float z) {
            return Vector3(x, y, z);
        }),
            py::arg("x") = 0.0f,
            py::arg("y") = 0.0f,
            py::arg("z") = 0.0f
        )
        .def("__repr__", [](const Vector3& v) {
            return "Vector3(" + std::to_string(v.x()) + ", " 
                              + std::to_string(v.y()) + ", " 
                              + std::to_string(v.z()) + ")";
        })
        .def("x", &Vector3::x)
        .def("y", &Vector3::y)
        .def("z", &Vector3::z)
        .def("u", &Vector3::u)
        .def("v", &Vector3::v)
        .def("w", &Vector3::w);

    py::class_<Vector<Color>>(m_vec, "VectorBaseColor");
    py::class_<Vec3Core<Color>, Vector<Color>>(m_vec, "Vec3CoreColor");
    py::class_<Color, Vec3Core<Color>, Vector<Color>>(m_vec, "Color")
        .def(py::init([](float r, float g, float b) {
            return Color(r, g, b);
        }),
            py::arg("r") = 0.0f,
            py::arg("g") = 0.0f,
            py::arg("b") = 0.0f
        )
        .def("__repr__", [](const Color& c) {
            return "Color(" + std::to_string(c.r()) + ", " 
                            + std::to_string(c.g()) + ", " 
                            + std::to_string(c.b()) + ")";
        })
        .def("r",      &Color::r)
        .def("g",      &Color::g)
        .def("b",      &Color::b)
        .def("black",  &Color::black)
        .def("white",  &Color::white)
        .def("red",    &Color::red)
        .def("green",  &Color::green)
        .def("blue",   &Color::blue)
        .def("purple", &Color::purple)
        .def("yellow", &Color::yellow)
        .def("teal",   &Color::teal);

    m_vec.def("dot",     &dot,     "");
    m_vec.def("cross",   &cross,   "");
    m_vec.def("reflect", &reflect, "");
    m_vec.def("refract", &refract, "");

    m_vec.def("random_unit_vector",   &random_unit_vector,   "");
    m_vec.def("random_on_hemisphere", &random_on_hemisphere, "");

    /* MATRICES */

    auto m_mat = m_core.def_submodule("matrix", "Matrix module.");

    py::class_<Matrix3>(m_mat, "Matrix3")
        .def(py::init<>())
        .def("transpose", &Matrix3::transpose)
        .def("T",         &Matrix3::T)
        .def("right",     &Matrix3::right)
        .def("up",        &Matrix3::up)
        .def("forward",   &Matrix3::forward);

    m_mat.def("rotation_from_euler", &rotation_from_euler, "");

}

