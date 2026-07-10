#pragma once

#include <pybind11/functional.h>

namespace py = pybind11;

// ############################################################################
// GIL CONTROLS
// ############################################################################

template<typename F, typename... Args>
void with_gil_scoped_acquire(F& func, Args... args) {
    {
        py::gil_scoped_acquire acquire;
        std::forward<F>(func)(std::forward<Args>(args)...);
    }
}

template<typename F, typename... Args>
void with_gil_scoped_release(F& func, Args... args) {
    {
        py::gil_scoped_release release;
        std::forward<F>(func)(std::forward<Args>(args)...);
    }
}

// ############################################################################
// HOOK / CALLBACK UTILITIES
// ############################################################################

template<typename... Args> 
inline void hook_no_op(Args&&... args) {};

