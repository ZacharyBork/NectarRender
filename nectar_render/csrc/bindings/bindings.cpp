#include <pybind11/pybind11.h>

#include "bindings/bind_core.h"
#include "bindings/bind_engine.h"

namespace py = pybind11;

PYBIND11_MODULE(_pathtracer, m) {
    m.doc() = "NectarRender C++ host module.";

    register_core(m);
    register_engine(m);

}


