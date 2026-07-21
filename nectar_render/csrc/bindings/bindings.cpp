#include <pybind11/pybind11.h>

#include "bindings/bind_core.h"
#include "bindings/bind_data.h"
#include "bindings/bind_engine.h"
#include "bindings/bind_hittable.h"
#include "bindings/bind_interface.h"
#include "bindings/bind_material.h"

#include "bindings/bind_host.h"

namespace py = pybind11;

PYBIND11_MODULE(_pathtracer, m) {
    
    m.doc() = "NectarRender C++ host module.";

    register_core(m);
    register_material(m);
    register_hittable(m);
    register_interface(m);
    register_data(m);
    register_engine(m);
    register_host(m);

}


