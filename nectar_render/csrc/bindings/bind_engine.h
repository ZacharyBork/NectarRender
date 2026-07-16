#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "engine/include/engine/engine.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/data.h"
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
// RENDER LAYERS
// ############################################################################

    auto m_data = m_engine.def_submodule("data", "Engine data submodule.");

    py::class_<DataObject>(m_data, "DataObject")
        .def("n_pixels",        &DataObject::n_pixels)
        .def("n_elements",      &DataObject::n_elements)
        .def("n_bytes",         &DataObject::n_bytes)
        .def("shape",           &DataObject::shape)
        .def("numpy",           &DataObject::numpy)
        .def("is_enabled",      &DataObject::is_enabled)
        .def("is_pinned",       &DataObject::is_pinned)
        .def("linear_to_gamma", &DataObject::linear_to_gamma)
        .def("tonemap",         &DataObject::tonemap)
        .def("device_ptr",      &DataObject::device_ptr)
        .def("readback_pinned", &DataObject::readback_pinned)
        .def_readonly("C",      &DataObject::C)
        .def_readonly("H",      &DataObject::H)
        .def_readonly("W",      &DataObject::W);

    py::enum_<LayerType>(m_engine, "LayerType")
        .value("BEAUTY",    LayerType::BEAUTY)
        .value("DIFFUSE",   LayerType::DIFFUSE)
        .value("SPECULAR",  LayerType::SPECULAR)
        .value("NORMAL",    LayerType::NORMAL)
        .value("SHADOW",    LayerType::SHADOW)
        .value("DEPTH",     LayerType::DEPTH)
        .value("EMISSION",  LayerType::EMISSION)
        .value("OBJECT_ID", LayerType::OBJECT_ID);

    py::class_<RenderLayersConfig>(m_data, "RenderLayersConfig")
        .def(py::init<>()) 
        .def(py::init([](
            bool beauty,
            bool diffuse,
            bool specular,
            bool normal,
            bool shadow,
            bool depth,
            bool emission,
            bool object_id
        ) {
            RenderLayersConfig cfg;
            cfg.beauty    = beauty;
            cfg.diffuse   = diffuse;
            cfg.specular  = specular;
            cfg.normal    = normal;
            cfg.shadow    = shadow;
            cfg.depth     = depth;
            cfg.emission  = emission;
            cfg.object_id = object_id;
            return cfg;
        }),
            py::arg("beauty")    = true,
            py::arg("diffuse")   = false,
            py::arg("specular")  = false,
            py::arg("normal")    = false,
            py::arg("shadow")    = false,
            py::arg("depth")     = false,
            py::arg("emission")  = false,
            py::arg("object_id") = false
        )
        .def_readwrite("beauty",     &RenderLayersConfig::beauty)
        .def_readwrite("diffuse",    &RenderLayersConfig::diffuse)
        .def_readwrite("specular",   &RenderLayersConfig::specular)
        .def_readwrite("normal",     &RenderLayersConfig::normal)
        .def_readwrite("shadow",     &RenderLayersConfig::shadow)
        .def_readwrite("depth",      &RenderLayersConfig::depth)
        .def_readwrite("emission",   &RenderLayersConfig::emission)
        .def_readwrite("object_id",  &RenderLayersConfig::object_id);

    py::class_<RenderLayers>(m_data, "RenderLayers")
        .def(py::init([](
            size_t h, 
            size_t w,
            const RenderLayersConfig& cfg
        ) {
            return RenderLayers(h, w, cfg);
        }),
            py::arg("h")   = true,
            py::arg("w")   = false,
            py::arg("cfg") = false
        )
        .def("get_layer",            &RenderLayers::get_layer)
        .def("pin_buffer",           &RenderLayers::pin_buffer)
        .def("normalize_by_samples", &RenderLayers::normalize_by_samples)
        .def_readonly("H",           &RenderLayers::H)
        .def_readonly("W",           &RenderLayers::W)
        .def_readonly("beauty",      &RenderLayers::beauty)
        .def_readonly("diffuse",     &RenderLayers::diffuse)
        .def_readonly("specular",    &RenderLayers::specular)
        .def_readonly("normal",      &RenderLayers::normal)
        .def_readonly("shadow",      &RenderLayers::shadow)
        .def_readonly("depth",       &RenderLayers::depth)
        .def_readonly("emission",    &RenderLayers::emission)
        .def_readonly("object_id",   &RenderLayers::object_id);

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
            const Texture& texture
        ) {
            return ObjectLight(obj, brightness, texture);
        }),
            py::arg("obj"),
            py::arg("brightness") = 35.0f,
            py::arg("texture") = ConstantTexture(Color(1.0f, 1.0f, 1.0f))
        );

// ############################################################################
// SCENE
// ############################################################################

    py::class_<Scene>(m_engine, "Scene")
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
        .value("COMBINE",    SampleMode::COMBINE);

    py::enum_<EngineState>(m_engine, "EngineState")
        .value("IDLE",      EngineState::IDLE)
        .value("RENDERING", EngineState::RENDERING);

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
        .def("set_scene", ([](RenderEngine& self, Scene scene) {
            self.set_scene(scene);
        }),
            py::arg("scene")
        )
        .def("sample", ([](
            RenderEngine& self, 
            uint32_t   sample_index,
            SampleMode mode
        ) {
            self.sample(sample_index, mode);
        }),
            py::arg("sample_idx") = 0u,
            py::arg("mode") = SampleMode::ACCUMULATE
        )
        .def("render", ([](RenderEngine& self, SampleMode mode) {
            py::gil_scoped_release release;
            self.render(mode);
        }),
            py::arg("mode") = SampleMode::ACCUMULATE
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

        .def_readwrite("on_render_started",  &RenderEngine::on_render_started)
        .def_readwrite("on_frame_finished",  &RenderEngine::on_frame_finished)
        .def_readwrite("on_render_finished", &RenderEngine::on_render_finished)
        .def_readwrite("on_stopped",         &RenderEngine::on_stopped)
        .def_readwrite("on_reset",           &RenderEngine::on_reset)

        .def("camera", &RenderEngine::camera, return_policy::reference)
        .def("layers", &RenderEngine::layers, return_policy::reference)
        .def("scene",  &RenderEngine::scene,  return_policy::reference)
        
        .def("request_stop",   &RenderEngine::request_stop)
        .def("reset",          &RenderEngine::reset)
        .def("get_state",      &RenderEngine::get_state)
        .def("is_idle",        &RenderEngine::is_idle)
        .def("is_rendering",   &RenderEngine::is_rendering)
        .def("n_samples",      &RenderEngine::n_samples)
        .def("set_n_samples",  &RenderEngine::set_n_samples)
        .def("max_depth",      &RenderEngine::max_depth)
        .def("set_max_depth",  &RenderEngine::set_max_depth)
        
        .def("screen_space_ray", &RenderEngine::screen_space_ray, 
             return_policy::reference);

}
