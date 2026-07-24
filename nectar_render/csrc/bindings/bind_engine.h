#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "engine/include/engine/engine.h"
#include "engine/include/engine/camera.h"
#include "core/include/core/transform.h"
#include "engine/include/engine/light.h"
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
        .def("update",     &Camera::update)
        .def("parameters", &Camera::parameters, return_policy::copy);

// ############################################################################
// LIGHTS
// ############################################################################

    auto m_lights = m_engine.def_submodule("lights", "Lights submodule.");

    py::class_<SkyLight>(m_lights, "SkyLight")
        .def(py::init<>())
        .def(py::init([](
            const Color& start_color, 
            const Color& end_color
        ) {
            return SkyLight(start_color, end_color);
        }),
            py::arg("start_color") = Color(1.0f, 1.0f, 1.0f),
            py::arg("end_color")   = Color(0.5f, 0.7f, 1.0f)
        )
        .def("black", &SkyLight::black);

    py::class_<Light, Hittable>(m_lights, "Light");

    py::class_<ObjectLight, Light>(m_lights, "ObjectLight")
        .def(py::init([](
            Hittable& obj,
            float brightness,
            const Color& albedo
        ) {
            return ObjectLight(obj, brightness, albedo);
        }),
            py::arg("obj"),
            py::arg("brightness") = 35.0f,
            py::arg("albedo") = Color(1.0f, 1.0f, 1.0f)
        )
        .def(py::init([](
            Hittable& obj,
            float brightness,
            std::shared_ptr<Texture> texture
        ) {
            return ObjectLight(obj, brightness, texture);
        }),
            py::arg("obj"),
            py::arg("brightness") = 35.0f,
            py::arg("texture") = Texture::from_color(Color::white())
        );

// ############################################################################
// SCENE
// ############################################################################

    py::classh<Scene>(m_engine, "Scene")
        .def(py::init([](
            py::list hittables,
            py::list lights,
            SkyLight skylight
        ) {
            std::vector<Hittable*> obj_ptrs;
            obj_ptrs.reserve(hittables.size());
            for (auto& item : hittables)
                obj_ptrs.push_back(item.cast<Hittable*>());

            std::vector<Light*> light_ptrs;
            light_ptrs.reserve(lights.size());
            for (auto& item : lights)
                light_ptrs.push_back(item.cast<Light*>());

            return Scene(obj_ptrs, light_ptrs, skylight);
        }),
            py::arg("hittables"), 
            py::arg("lights"), 
            py::arg("skylight"),
            py::keep_alive<1, 2>(),
            py::keep_alive<1, 3>()
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

    py::enum_<SampleMode>(m_engine, "SampleMode")
        .value("ACCUMULATE", SampleMode::ACCUMULATE)
        .value("COMBINE",    SampleMode::COMBINE)
        .def("__repr__", [](const SampleMode& mode) {
            switch (mode) {
                case SampleMode::ACCUMULATE: return "SampleMode.ACCUMULATE";
                case SampleMode::COMBINE:    return "SampleMode.COMBINE";
            }
            return "SampleMode.UNKNOWN";
        });

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

        .def("queue_function", ([](
            RenderEngine& self, 
            std::function<void()> func,
            bool rebuild_bvh,
            bool immediate
        ) {
            self.queue_function(func, rebuild_bvh, immediate);
        }),
            py::arg("func"),
            py::arg("rebuild_bvh") = false,
            py::arg("immediate")   = true
        )

        .def("set_scene", [](
            RenderEngine& self,
            std::unique_ptr<Scene> scene
        ) {
            self.set_scene(std::move(*scene));
        })

        .def("render", ([](RenderEngine& self) {
            py::gil_scoped_release release;
            self.render();
        }))

        .def_readwrite("on_render_started",  &RenderEngine::on_render_started)
        .def_readwrite("on_frame_finished",  &RenderEngine::on_frame_finished)
        .def_readwrite("on_render_finished", &RenderEngine::on_render_finished)
        .def_readwrite("on_stopped",         &RenderEngine::on_stopped)
        .def_readwrite("on_reset",           &RenderEngine::on_reset)

        .def("camera", &RenderEngine::camera, return_policy::reference)
        .def("layers", &RenderEngine::layers, return_policy::reference)
        .def("scene",  &RenderEngine::scene,  return_policy::reference)
        .def("stream", &RenderEngine::stream, return_policy::reference)

        .def("set_sample_mode", &RenderEngine::set_sample_mode)
        .def("request_stop",    &RenderEngine::request_stop)
        .def("reset",           &RenderEngine::reset)
        
        .def("get_state",      &RenderEngine::get_state)
        .def("is_idle",        &RenderEngine::is_idle)
        .def("is_rendering",   &RenderEngine::is_rendering)
        
        .def("n_samples",      &RenderEngine::n_samples)
        .def("set_n_samples",  &RenderEngine::set_n_samples)
        .def("max_depth",      &RenderEngine::max_depth)
        .def("set_max_depth",  &RenderEngine::set_max_depth)
        
        
        .def("screen_space_ray",    &RenderEngine::screen_space_ray)
        .def("get_scene_interface", &RenderEngine::get_scene_interface, 
             return_policy::reference);

}
