#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "data/include/data.h"

namespace py = pybind11;
using return_policy = py::return_value_policy;

void register_data(py::module_& m) {
    
    auto m_data = m.def_submodule("data", "Data module.");

    py::enum_<StreamState>(m_data, "StreamState")
        .value("ACTIVE",   StreamState::ACTIVE)
        .value("INACTIVE", StreamState::INACTIVE)
        .def("__repr__", [](const StreamState& state) {
            switch (state) {
                case StreamState::ACTIVE:   return "StreamState.ACTIVE";
                case StreamState::INACTIVE: return "StreamState.INACTIVE";
            }
            return "StreamState.UNKNOWN";
        });

    py::enum_<TonemapMethod>(m_data, "TonemapMethod")
        .value("REINHARD",   TonemapMethod::REINHARD)
        .def("__repr__", [](const TonemapMethod& method) {
            switch (method) {
                case TonemapMethod::REINHARD: return "StreamState.REINHARD";
            }
            return "TonemapMethod.UNKNOWN";
        });

    py::class_<StreamConfig>(m_data, "StreamConfig")
        .def(py::init<>())
        .def_readwrite("linear_to_gamma",   &StreamConfig::linear_to_gamma)
        .def_readwrite("apply_tonemapping", &StreamConfig::apply_tonemapping)
        .def_readwrite("tonemap_method",    &StreamConfig::tonemap_method)
        .def_readwrite("tonemap_alpha",     &StreamConfig::tonemap_alpha);

    py::class_<TransferStream>(m_data, "TransferStream")
        .def("update_config",  &TransferStream::update_config)
        .def("buffer_ptr",     &TransferStream::buffer_ptr)
        .def("readback",       &TransferStream::readback)
        .def("get_state",      &TransferStream::get_state)
        .def("is_active",      &TransferStream::is_active)
        .def("is_inactive",    &TransferStream::is_inactive)
        .def("is_linked",      &TransferStream::is_linked)
        .def("has_overlay",    &TransferStream::has_overlay)
        .def("remove_overlay", &TransferStream::remove_overlay)
        .def("shape",          &TransferStream::shape)
        .def("n_pixels",       &TransferStream::n_pixels)
        .def("n_elements",     &TransferStream::n_elements)
        .def("n_bytes",        &TransferStream::n_bytes);

    py::class_<DataObject>(m_data, "DataObject")
        .def("n_pixels",        &DataObject::n_pixels)
        .def("n_elements",      &DataObject::n_elements)
        .def("n_bytes",         &DataObject::n_bytes)
        .def("shape",           &DataObject::shape)
        .def("numpy",           &DataObject::numpy)
        .def("is_enabled",      &DataObject::is_enabled)
        .def("device_ptr",      &DataObject::device_ptr)
        .def_readonly("C",      &DataObject::C)
        .def_readonly("H",      &DataObject::H)
        .def_readonly("W",      &DataObject::W);

    py::enum_<LayerType>(m_data, "LayerType")
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

}

