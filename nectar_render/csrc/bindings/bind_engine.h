#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "engine/include/engine/engine.h"
#include "engine/include/engine/camera.h"

namespace py = pybind11;

void register_engine(py::module_& m) {
    
    auto m_engine = m.def_submodule("engine", "Engine module.");

    py::class_<CameraParams>(m_engine, "CameraParams")
        .def(py::init<>()) 
        .def(py::init([](
            std::array<int,   2> resolution,
            std::array<float, 3> position,
            std::array<float, 3> rotation,
            float focal_length,
            float focus_distance,
            float aperture,
            float sensor_width,
            float shutter_speed
        ) {
            CameraParams p;
            p.resolution    = resolution;
            p.position      = position;
            p.rotation      = rotation;
            p.focal_length   = focal_length;
            p.focus_distance = focus_distance;
            p.aperture       = aperture;
            p.sensor_width   = sensor_width;
            p.shutter_speed  = shutter_speed;
            return p;
        }),
            py::arg("resolution")    = std::array<int,   2>{512, 512},
            py::arg("position")      = std::array<float, 3>{0.0f, 0.0f, 0.0f},
            py::arg("rotation")      = std::array<float, 3>{0.0f, 0.0f, 0.0f},
            py::arg("focal_length")   = 5.0f,
            py::arg("focus_distance") = 10.0f,
            py::arg("aperture")       = 0.01f,
            py::arg("sensor_width")   = 2.0f,
            py::arg("shutter_speed")  = 1.0f
        )
        .def_readwrite("resolution",     &CameraParams::resolution)
        .def_readwrite("position",       &CameraParams::position)
        .def_readwrite("rotation",       &CameraParams::rotation)
        .def_readwrite("focal_length",   &CameraParams::focal_length)
        .def_readwrite("focus_distance", &CameraParams::focus_distance)
        .def_readwrite("aperture",       &CameraParams::aperture)
        .def_readwrite("sensor_width",   &CameraParams::sensor_width)
        .def_readwrite("shutter_speed",  &CameraParams::shutter_speed);

    m_engine.def("initialize", [](
            CameraParams camera_params,
            unsigned int samples   = 10,
            unsigned int ray_depth = 8,
            unsigned int seed      = 54321
        ) { 
            engine.initialize(camera_params, samples, ray_depth, seed); 
        }
    );

    m_engine.def("render", []() { return engine.render(); });

}

