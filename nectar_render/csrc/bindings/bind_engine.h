#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "engine/include/engine/requests.h"
#include "engine/include/engine/engine.h"
#include "engine/include/engine/camera.h"
#include "core/include/core/transform.h"
#include "engine/include/engine/denoise.h"

namespace py = pybind11;
using return_policy = py::return_value_policy;

void register_engine(py::module_& m) {
    
    auto m_engine = m.def_submodule("engine", "Engine module.");

// ############################################################################
// CAMERA
// ############################################################################

    auto m_camera = m_engine.def_submodule("camera", "Camera submodule.");

    py::class_<CameraParams>(m_camera, "CameraParams")
        .def(py::init([](
            Vector2  resolution,
            Vector3  position,
            Vector3  rotation,
            uint32_t samples_per_pixel,
            float    focal_length,
            float    focus_distance,
            float    aperture,
            float    sensor_width,
            float    shutter_speed
        ) {
            return CameraParams{
                resolution, position, rotation, samples_per_pixel, 
                focal_length, focus_distance, aperture, sensor_width,
                shutter_speed
            };
        }),
            py::arg("resolution"),
            py::arg("position"),
            py::arg("rotation"),
            py::arg("samples_per_pixel"),
            py::arg("focal_length"),
            py::arg("focus_distance"),
            py::arg("aperture"),
            py::arg("sensor_width"),
            py::arg("shutter_speed")
        )
        .def_readonly("R",                  &CameraParams::R)
        .def_readonly("shutter_time",       &CameraParams::shutter_time)
        .def_readonly("aspect_ratio",       &CameraParams::aspect_ratio)
        .def_readwrite("resolution",        &CameraParams::resolution)
        .def_readwrite("position",          &CameraParams::position)
        .def_readwrite("rotation",          &CameraParams::rotation)
        .def_readwrite("samples_per_pixel", &CameraParams::samples_per_pixel)
        .def_readwrite("focal_length",      &CameraParams::focal_length)
        .def_readwrite("focus_distance",    &CameraParams::focus_distance)
        .def_readwrite("aperture",          &CameraParams::aperture)
        .def_readwrite("sensor_width",      &CameraParams::sensor_width)
        .def_readwrite("shutter_speed",     &CameraParams::shutter_speed);

    py::class_<Camera>(m_camera, "Camera")
        .def(py::init([](CameraParams params) {
            return Camera(params);
        }),
            py::arg("params")
        )
        .def_readwrite("on_updated", &Camera::on_updated)
        .def("parameters", &Camera::parameters, return_policy::copy)
        .def("project_to_screen", &Camera::project_to_screen, 
            py::arg("world_point")
        );

// ############################################################################
// DENOISERS
// ############################################################################

    auto m_denoise = m_engine.def_submodule("denoise", "Denoising submodule.");

    py::class_<Denoiser>(m_denoise, "Denoiser").def("run", &Denoiser::run);

    py::class_<TVDenoiser, Denoiser>(m_denoise, "TVDenoiser")
        .def(py::init([](
            const float weight, 
            const uint32_t iterations
        ) {
            return TVDenoiser(weight, iterations);
        }),
            py::arg("weight")     = 1.0f,
            py::arg("iterations") = 100
        )
        .def("run", &TVDenoiser::run);

// ############################################################################
// RENDER ENGINE CLASS
// ############################################################################

    py::enum_<EngineState>(m_engine, "EngineState")
        .value("IDLE",      EngineState::IDLE)
        .value("RENDERING", EngineState::RENDERING)
        .def("__repr__", [](const EngineState& state) {
            switch (state) {
                case EngineState::IDLE:      return "EngineState.IDLE";
                case EngineState::RENDERING: return "EngineState.RENDERING";
            }
            return "EngineState.UNKNOWN";
        });

    py::enum_<EngineType>(m_engine, "EngineType")
        .value("PATHTRACER", EngineType::PATHTRACER)
        .value("VIEWPORT",   EngineType::VIEWPORT)
        .def("__repr__", [](const EngineType& type) {
            switch (type) {
                case EngineType::PATHTRACER: return "EngineType.PATHTRACER";
                case EngineType::VIEWPORT:   return "EngineType.VIEWPORT";
            }
            return "EngineType.UNKNOWN";
        });

    py::class_<EnginePollResponse>(m_engine, "EnginePollResponse")
        .def(py::init<>())
        .def_readwrite(
            "should_update_camera", &EnginePollResponse::should_update_camera
        )
        .def_readwrite("camera_params", &EnginePollResponse::camera_params);

    py::class_<EngineRequests>(m_engine, "EngineRequests")
        .def("start",             &EngineRequests::start)
        .def("stop",              &EngineRequests::stop)
        .def("restart",           &EngineRequests::restart)
        .def("shutdown",          &EngineRequests::shutdown);

    py::class_<RenderEngine>(m_engine, "RenderEngine")
        .def(py::init([](
            Camera&  camera,
            uint32_t ray_depth,
            uint32_t seed
        ) { 
            return std::make_unique<RenderEngine>(camera, ray_depth, seed);
        }),
            py::arg("camera"),
            py::arg("ray_depth") = 8u,
            py::arg("seed")      = 54321u
        )

        .def("set_scene", [](
            RenderEngine& self,
            std::unique_ptr<Scene> scene
        ) {
            self.set_scene(std::move(*scene));
        })

        .def("idle", ([](RenderEngine& self) {
            py::gil_scoped_release release;
            self.idle();
        }))

        .def("render", ([](RenderEngine& self) {
            py::gil_scoped_release release;
            self.render();
        }))

        .def_readwrite("on_render_started",  &RenderEngine::on_render_started)
        .def_readwrite("on_frame_finished",  &RenderEngine::on_frame_finished)
        .def_readwrite("on_render_finished", &RenderEngine::on_render_finished)
        .def_readwrite("on_stopped",         &RenderEngine::on_stopped)
        .def_readwrite("on_restarted",       &RenderEngine::on_restarted)
        .def_readwrite("on_reset",           &RenderEngine::on_reset)
        .def_readwrite("on_shutdown",        &RenderEngine::on_shutdown)
        .def_readwrite("poll_updates",       &RenderEngine::poll_updates)

        .def("camera",   &RenderEngine::camera,   return_policy::reference)
        .def("layers",   &RenderEngine::layers,   return_policy::reference)
        .def("scene",    &RenderEngine::scene,    return_policy::reference)
        .def("stream",   &RenderEngine::stream,   return_policy::reference)
        .def("requests", &RenderEngine::requests, return_policy::reference)

        .def("get_state",        &RenderEngine::get_state)
        .def("is_idle",          &RenderEngine::is_idle)
        .def("is_rendering",     &RenderEngine::is_rendering)

        .def("get_engine_type",  &RenderEngine::get_engine_type)
        .def("set_engine_type",  &RenderEngine::set_engine_type)
        
        .def("n_samples",        &RenderEngine::n_samples)
        .def("set_n_samples",    &RenderEngine::set_n_samples)
        .def("max_depth",        &RenderEngine::max_depth)
        .def("set_max_depth",    &RenderEngine::set_max_depth)

        .def("set_axis_grid_visible", &RenderEngine::set_axis_grid_visible)

        .def("get_scene_interface", &RenderEngine::get_scene_interface, 
             return_policy::reference);

}
