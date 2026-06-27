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

    /* VECTORS */

    auto m_vector = m_core.def_submodule("vector", "Vector module.");
    
    py::class_<Vector2>(m_vector, "Vector2")
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
        .def("x", &Vector2::x)
        .def("y", &Vector2::y)
        .def("u", &Vector2::u)
        .def("v", &Vector2::v);

    py::class_<Vector3>(m_vector, "Vector3")
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
        .def("x", &Vector3::x)
        .def("y", &Vector3::y)
        .def("z", &Vector3::z)
        .def("u", &Vector3::u)
        .def("v", &Vector3::v)
        .def("w", &Vector3::w);

    py::class_<Color>(m_vector, "Color")
        .def(py::init([](float r, float g, float b) {
            return Color(r, g, b);
        }),
            py::arg("r") = 0.0f,
            py::arg("g") = 0.0f,
            py::arg("b") = 0.0f
        )
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

    auto m_matrix = m_core.def_submodule("matrix", "Matrix module.");

    py::class_<Matrix3>(m_matrix, "Matrix3")
        .def(py::init<>())
        .def("transpose", &Matrix3::transpose)
        .def("T",         &Matrix3::T)
        .def("right",     &Matrix3::right)
        .def("up",        &Matrix3::up)
        .def("forward",   &Matrix3::forward);

    m_matrix.def("rotation_from_euler", &rotation_from_euler, "");

}

