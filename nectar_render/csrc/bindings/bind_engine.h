#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "engine/include/engine/engine.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/data.h"
#include "engine/include/engine/transform.h"
#include "engine/include/engine/light.h"

namespace py = pybind11;

void register_engine(py::module_& m) {
    
    auto m_engine = m.def_submodule("engine", "Engine module.");

// ############################################################################
// TRANSFORM
// ############################################################################

    py::class_<Transform>(m_engine, "Transform")
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
        

// ############################################################################
// CAMERA
// ############################################################################

    auto m_camera = m_engine.def_submodule("camera", "Camera submodule.");

    py::class_<CameraParams>(m_camera, "CameraParams")
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
            p.resolution     = resolution;
            p.position       = position;
            p.rotation       = rotation;
            p.focal_length   = focal_length;
            p.focus_distance = focus_distance;
            p.aperture       = aperture;
            p.sensor_width   = sensor_width;
            p.shutter_speed  = shutter_speed;
            return p;
        }),
            py::arg("resolution") = std::array<int,   2>{512, 512},
            py::arg("position")   = std::array<float, 3>{0.0f, 0.0f, 0.0f},
            py::arg("rotation")   = std::array<float, 3>{0.0f, 0.0f, 0.0f},
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

    py::class_<Camera>(m_camera, "Camera")
        .def(py::init([](const CameraParams& p) {
            return Camera(p);
        }),
            py::arg("p") = CameraParams()
        )
        .def_readwrite("resolution",     &Camera::resolution)
        .def_readwrite("position",       &Camera::position)
        .def_readwrite("rotation",       &Camera::rotation)
        .def_readwrite("focal_length",   &Camera::focal_length)
        .def_readwrite("focus_distance", &Camera::focus_distance)
        .def_readwrite("aperture",       &Camera::aperture)
        .def_readwrite("sensor_width",   &Camera::sensor_width)
        .def_readwrite("shutter_speed",  &Camera::shutter_speed);

// ############################################################################
// RENDER LAYERS
// ############################################################################

    auto m_data = m_engine.def_submodule("data", "Engine data submodule.");

    py::class_<DataObject>(m_data, "DataObject")
        .def("numpy",               &DataObject::numpy)
        .def("is_enabled",          &DataObject::is_enabled)
        .def("linear_to_gamma",     &DataObject::linear_to_gamma)
        .def("tonemap",             &DataObject::tonemap)
        .def_readonly("device_ptr", &DataObject::device_ptr)
        .def_readonly("C",          &DataObject::C)
        .def_readonly("H",          &DataObject::H)
        .def_readonly("W",          &DataObject::W)
        .def_readonly("n_elements", &DataObject::n_elements);

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
        .def_readwrite("beauty",    &RenderLayersConfig::beauty)
        .def_readwrite("diffuse",   &RenderLayersConfig::diffuse)
        .def_readwrite("specular",  &RenderLayersConfig::specular)
        .def_readwrite("normal",    &RenderLayersConfig::normal)
        .def_readwrite("shadow",    &RenderLayersConfig::shadow)
        .def_readwrite("depth",     &RenderLayersConfig::depth)
        .def_readwrite("emission",  &RenderLayersConfig::emission)
        .def_readwrite("object_id", &RenderLayersConfig::object_id);

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
        .def_readonly("H",         &RenderLayers::H)
        .def_readonly("W",         &RenderLayers::W)
        .def_readonly("beauty",    &RenderLayers::beauty)
        .def_readonly("diffuse",   &RenderLayers::diffuse)
        .def_readonly("specular",  &RenderLayers::specular)
        .def_readonly("normal",    &RenderLayers::normal)
        .def_readonly("shadow",    &RenderLayers::shadow)
        .def_readonly("depth",     &RenderLayers::depth)
        .def_readonly("emission",  &RenderLayers::emission)
        .def_readonly("object_id", &RenderLayers::object_id);

// ############################################################################
// LIGHTS
// ############################################################################

    auto m_lights = m_engine.def_submodule("lights", "Lights submodule.");

    py::class_<Light>(m_lights, "Light");

    py::class_<SkyLight, Light>(m_lights, "SkyLight")
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

// ############################################################################
// SCENE
// ############################################################################

    py::class_<Scene>(m_engine, "Scene")
        .def(py::init([](
            py::list hittables,
            SkyLight skylight
        ) {
            std::vector<Hittable*> ptrs;
            ptrs.reserve(hittables.size());
            for (auto& item : hittables)
                ptrs.push_back(item.cast<Hittable*>());

            return Scene(ptrs, skylight);
        }),
            py::arg("hittables"), 
            py::arg("skylight")
        );

// ############################################################################
// RENDER ENGINE CLASS
// ############################################################################

    py::class_<RenderEngine>(m_engine, "RenderEngine")
        .def(py::init<>())
        .def("initialize", [](
            RenderEngine& self,
            Camera        camera,
            unsigned int  samples   = 10,
            unsigned int  ray_depth = 8,
            unsigned int  seed      = 54321
        ) { 
            self.initialize(camera, samples, ray_depth, seed); 
        })
        .def("render", [](RenderEngine& self, Scene& scene) {
            self.render(scene);
        })
        .def("get_render_layers",           &RenderEngine::get_render_layers)
        .def_readonly("num_samples",        &RenderEngine::num_samples)
        .def_readonly("max_depth",          &RenderEngine::max_depth)
        .def_readwrite("on_frame_finished", &RenderEngine::on_frame_finished);
}

