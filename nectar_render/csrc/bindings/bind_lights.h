#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "light/include/skylight.h"

namespace py = pybind11;

void register_light(py::module_& m) {
    
    auto m_lights = m.def_submodule("light", "Lights module.");
    
    py::enum_<SkylightType>(m_lights, "SkylightType")
        .value("Null",   SkylightType::Null)
        .value("Simple", SkylightType::Simple)
        .value("HDRI",   SkylightType::HDRI)
        .def("__repr__", [](const SkylightType& type) {
            switch (type) {
                case SkylightType::Null:   return "SkylightType.Null";
                case SkylightType::Simple: return "SkylightType.Simple";
                case SkylightType::HDRI:   return "SkylightType.HDRI";
            }
            return "SkylightType.UNKNOWN";
        });

    py::class_<SimpleSkylightConfig>(m_lights, "SimpleSkylightConfig")
        .def_readwrite("start", &SimpleSkylightConfig::start)
        .def_readwrite("end",   &SimpleSkylightConfig::end);

    py::class_<HDRISkylightConfig>(m_lights, "HDRISkylightConfig")
        .def_readwrite("rotation",  &HDRISkylightConfig::rotation)
        .def_readwrite("intensity", &HDRISkylightConfig::intensity);

    py::class_<Sky::Simple>(m_lights, "Simple");
    py::class_<Sky::HDRI>  (m_lights, "HDRI");

    py::classh<Skylight>(m_lights, "Skylight")
        .def(py::init<>())
        .def_static("simple", &Skylight::simple,
            py::arg("start_color") = Color(1.0f, 1.0f, 1.0f),
            py::arg("end_color")   = Color(0.5f, 0.7f, 1.0f)
        )
        .def_static("hdri", py::overload_cast<>(&Skylight::hdri))
        .def_static("hdri",
            py::overload_cast<const std::string&>(&Skylight::hdri),
            py::arg("filepath")
        )
        .def("load_hdri_file", &Skylight::load_hdri_file)
        .def("config_simple", &Skylight::config_simple, 
            py::return_value_policy::reference
        )
        .def("config_hdri", &Skylight::config_hdri, 
            py::return_value_policy::reference
        );

}
