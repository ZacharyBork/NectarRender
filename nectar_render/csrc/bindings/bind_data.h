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
        .value("FROZEN",   StreamState::INACTIVE)
        .def("__repr__", [](const StreamState& state) {
            switch (state) {
                case StreamState::ACTIVE:   return "StreamState.ACTIVE";
                case StreamState::INACTIVE: return "StreamState.INACTIVE";
                case StreamState::FROZEN:   return "StreamState.FROZEN";
            }
            return "StreamState.UNKNOWN";
        });

    py::enum_<TonemapMethod>(m_data, "TonemapMethod")
        .value("REINHARD",          TonemapMethod::REINHARD)
        .value("REINHARD_EXTENDED", TonemapMethod::REINHARD_EXTENDED)
        .value("ACES",              TonemapMethod::ACES)
        .def("__repr__", [](const TonemapMethod& method) {
            switch (method) {
                case TonemapMethod::REINHARD:
                    return "TonemapMethod.REINHARD";
                case TonemapMethod::REINHARD_EXTENDED:
                    return "TonemapMethod.REINHARD_EXTENDED";
                case TonemapMethod::ACES:
                    return "TonemapMethod.ACES";
            }
            return "TonemapMethod.UNKNOWN";
        });

    py::enum_<DenoiseFilterQuality>(m_data, "DenoiseFilterQuality")
        .value("DEFAULT",  DenoiseFilterQuality::DEFAULT)
        .value("HIGH",     DenoiseFilterQuality::HIGH)
        .value("BALANCED", DenoiseFilterQuality::BALANCED)
        .value("FAST",     DenoiseFilterQuality::FAST)
        .def("__repr__", [](const DenoiseFilterQuality& quality) {
            switch (quality) {
                case DenoiseFilterQuality::DEFAULT:   
                    return "DenoiseFilterQuality.DEFAULT";
                case DenoiseFilterQuality::HIGH: 
                    return "DenoiseFilterQuality.BALANCED";
                case DenoiseFilterQuality::BALANCED:   
                    return "DenoiseFilterQuality.FROZEN";
                case DenoiseFilterQuality::FAST:   
                    return "DenoiseFilterQuality.FAST";
            }
            return "DenoiseFilterQuality.UNKNOWN";
        });

    py::class_<StreamConfig>(m_data, "StreamConfig")
        .def(py::init<>())
        .def_readwrite("apply_denoising", &StreamConfig::apply_denoising)
        .def_readwrite("denoise_clean_auxiliaries", 
                       &StreamConfig::denoise_clean_auxiliaries)
        .def_readwrite("denoise_quality", &StreamConfig::denoise_quality)
        .def_readwrite("denoise_input_scale",
                       &StreamConfig::denoise_input_scale)

        .def_readwrite("linear_to_gamma",   &StreamConfig::linear_to_gamma)
        .def_readwrite("apply_white_balance", 
                       &StreamConfig::apply_white_balance)
        .def_readwrite("wb_temperature",    &StreamConfig::wb_temperature)
        .def_readwrite("wb_tint",           &StreamConfig::wb_tint)
        .def_readwrite("apply_tonemapping", &StreamConfig::apply_tonemapping)
        .def_readwrite("tm_method",         &StreamConfig::tm_method)
        .def_readwrite("tm_white_point",    &StreamConfig::tm_white_point)
        .def_readwrite("tm_alpha",          &StreamConfig::tm_alpha);

    py::class_<TransferStream>(m_data, "TransferStream")
        .def("readback", [](TransferStream& stream) {
            py::gil_scoped_release release;
            return stream.readback();
        })
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
        .def("n_bytes",        &TransferStream::n_bytes)
        .def("enable_denoising",  &TransferStream::enable_denoising)
        .def("disable_denoising", &TransferStream::disable_denoising)
        .def("rebuild_denoiser",  &TransferStream::rebuild_denoiser);

    py::class_<DataObject>(m_data, "DataObject")
        .def("n_pixels",   &DataObject::n_pixels)
        .def("n_elements", &DataObject::n_elements)
        .def("n_bytes",    &DataObject::n_bytes)
        .def("shape",      &DataObject::shape)
        .def("numpy",      &DataObject::numpy)
        .def("is_enabled", &DataObject::is_enabled)
        .def("device_ptr", &DataObject::device_ptr)
        .def_readonly("C", &DataObject::C)
        .def_readonly("H", &DataObject::H)
        .def_readonly("W", &DataObject::W);

    py::enum_<LayerType>(m_data, "LayerType")
        .value("BEAUTY",        LayerType::BEAUTY)
        .value("DIFFUSE",       LayerType::DIFFUSE)
        .value("WORLD_NORMAL",  LayerType::WORLD_NORMAL)
        .value("OBJECT_NORMAL", LayerType::OBJECT_NORMAL)
        .value("SPECULAR",      LayerType::SPECULAR)
        .value("SHADOW",        LayerType::SHADOW)
        .value("DEPTH",         LayerType::DEPTH)
        .value("EMISSION",      LayerType::EMISSION)
        .value("OBJECT_ID",     LayerType::OBJECT_ID)
        .def("__repr__", [](const LayerType& type) {
            switch (type) {
                case LayerType::BEAUTY:        return "LayerType.BEAUTY";
                case LayerType::DIFFUSE:       return "LayerType.DIFFUSE";
                case LayerType::WORLD_NORMAL:  return "LayerType.WORLD_NORMAL";
                case LayerType::OBJECT_NORMAL: return "LayerType.OBJECT_NORMAL";
                case LayerType::SPECULAR:      return "LayerType.SPECULAR";
                case LayerType::SHADOW:        return "LayerType.SHADOW";
                case LayerType::DEPTH:         return "LayerType.DEPTH";
                case LayerType::EMISSION:      return "LayerType.EMISSION";
                case LayerType::OBJECT_ID:     return "LayerType.OBJECT_ID";
            }
            return "LayerType.UNKNOWN";
        });

    py::class_<RenderLayersConfig>(m_data, "RenderLayersConfig")
        .def(py::init<>()) 
        .def(py::init([](
            bool beauty,
            bool diffuse,
            bool world_normal,
            bool object_normal,
            bool specular,
            bool shadow,
            bool depth,
            bool emission,
            bool object_id
        ) {
            RenderLayersConfig cfg;
            cfg.beauty        = beauty;
            cfg.diffuse       = diffuse;
            cfg.world_normal  = world_normal;
            cfg.object_normal = object_normal;
            cfg.specular      = specular;
            cfg.shadow        = shadow;
            cfg.depth         = depth;
            cfg.emission      = emission;
            cfg.object_id     = object_id;
            return cfg;
        }),
            py::arg("beauty")        = true,
            py::arg("diffuse")       = true,
            py::arg("world_normal")  = true,
            py::arg("object_normal") = false,
            py::arg("specular")      = false,
            py::arg("shadow")        = false,
            py::arg("depth")         = false,
            py::arg("emission")      = false,
            py::arg("object_id")     = false
        )
        .def_readwrite("beauty",        &RenderLayersConfig::beauty)
        .def_readwrite("diffuse",       &RenderLayersConfig::diffuse)
        .def_readwrite("world_normal",  &RenderLayersConfig::world_normal)
        .def_readwrite("object_normal", &RenderLayersConfig::object_normal)
        .def_readwrite("specular",      &RenderLayersConfig::specular)
        .def_readwrite("shadow",        &RenderLayersConfig::shadow)
        .def_readwrite("depth",         &RenderLayersConfig::depth)
        .def_readwrite("emission",      &RenderLayersConfig::emission)
        .def_readwrite("object_id",     &RenderLayersConfig::object_id);

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
        .def("get_layer",              &RenderLayers::get_layer)
        .def("normalize_by_samples",   &RenderLayers::normalize_by_samples)
        .def_readonly("H",             &RenderLayers::H)
        .def_readonly("W",             &RenderLayers::W)
        .def_readonly("beauty",        &RenderLayers::beauty)
        .def_readonly("diffuse",       &RenderLayers::diffuse)
        .def_readonly("world_normal",  &RenderLayers::world_normal)
        .def_readonly("object_normal", &RenderLayers::object_normal)
        .def_readonly("specular",      &RenderLayers::specular)
        .def_readonly("shadow",        &RenderLayers::shadow)
        .def_readonly("depth",         &RenderLayers::depth)
        .def_readonly("emission",      &RenderLayers::emission)
        .def_readonly("object_id",     &RenderLayers::object_id);

}

