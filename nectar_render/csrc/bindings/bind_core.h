#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include "core/include/core.h"
#include "core/include/core/cublas_context.h"

namespace py = pybind11;

void register_core(py::module_& m) {
    
    auto m_core = m.def_submodule("core", "Core module.");

// ############################################################################
// RNG
// ############################################################################

    auto m_random = m_core.def_submodule("random", "Core RNG submodule.");
    py::class_<Generator>(m_random, "Generator");

// ############################################################################
// VECTORS
// ############################################################################

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
        .def("as_array", &Vector3::as_array)
        .def("x", &Vector3::x)
        .def("y", &Vector3::y)
        .def("z", &Vector3::z)
        .def("u", &Vector3::u)
        .def("v", &Vector3::v)
        .def("w", &Vector3::w)
        .def(py::self + py::self).def(py::self += py::self)
        .def(py::self +  float()).def(py::self +=  float())
        .def(py::self - py::self).def(py::self -= py::self)
        .def(py::self -  float()).def(py::self -=  float())
        .def(py::self * py::self).def(py::self *= py::self)
        .def(py::self *  float()).def(py::self *=  float())
        .def(py::self / py::self).def(py::self /= py::self)
        .def(py::self /  float()).def(py::self /=  float());

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

// ############################################################################
// MATRICES
// ############################################################################

    auto m_mat = m_core.def_submodule("matrix", "Matrix module.");

    py::class_<Matrix3>(m_mat, "Matrix3")
        .def(py::init<>())
        .def("transpose", &Matrix3::transpose)
        .def("T",         &Matrix3::T)
        .def("right",     &Matrix3::right)
        .def("up",        &Matrix3::up)
        .def("forward",   &Matrix3::forward)
        .def("numpy",     &Matrix3::numpy);

    m_mat.def("rotation_from_euler", &rotation_from_euler, "");

// ############################################################################
// TRANSFORM
// ############################################################################

    py::class_<Transform>(m_core, "Transform")
        .def(py::init<>()) 
        .def(py::init([](
            const Vector3& position,
            const Vector3& rotation,
            const Vector3& scale
        ) {
            return Transform(position, rotation, scale);
        }),
            py::arg("position") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("rotation") = Vector3(0.0f, 0.0f, 0.0f),
            py::arg("scale")    = Vector3(1.0f, 1.0f, 1.0f)
        )
        .def(py::init([](
            std::array<float, 3> position,
            std::array<float, 3> rotation,
            std::array<float, 3> scale
        ) {
            return Transform(position, rotation, scale);
        }),
            py::arg("position") = std::array<float, 3>{0.0f, 0.0f, 0.0f},
            py::arg("rotation") = std::array<float, 3>{0.0f, 0.0f, 0.0f},
            py::arg("scale")    = std::array<float, 3>{1.0f, 1.0f, 1.0f}
        )
        .def("position",     &Transform::position)
        .def("rotation",     &Transform::rotation)
        .def("scale",        &Transform::scale)
        .def("set_position", &Transform::set_position)
        .def("set_rotation", &Transform::set_rotation)
        .def("set_scale",    &Transform::set_scale)
        .def("p",            &Transform::p)
        .def("pos",          &Transform::pos)
        .def("R",            &Transform::R);

}

