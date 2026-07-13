#pragma once

#include <pybind11/pybind11.h>
#include "interface/include/object_interface.h"

namespace py = pybind11;
using return_policy = py::return_value_policy;

void register_interface(py::module_& m) {
    
    auto m_interface = m.def_submodule("interface", "Interface module.");

    py::class_<ObjectInterface>(m_interface, "ObjectInterface")
        .def("update_material", [](
            ObjectInterface& self, 
            const Material& mat
        ) {
            self.update_material(mat);
        },
            py::arg("mat")
        )
        .def("get_material", &ObjectInterface::get_material)
        .def("is_enabled",   &ObjectInterface::is_enabled)
        .def("hit_record",   &ObjectInterface::hit_record, 
             return_policy::reference)
        
        ;

}

