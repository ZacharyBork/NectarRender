#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <cuda_runtime.h>
#include <stdint.h>

namespace py = pybind11;

/* MEMORY MANAGEMENT */

uintptr_t allocate_cuda_memory(size_t n_elements, float fill_value);
void free_cuda_memory(uintptr_t device_ptr);

/* DATA TRANSFER */

py::array to_numpy(uintptr_t device_ptr, std::vector<size_t> shape);
