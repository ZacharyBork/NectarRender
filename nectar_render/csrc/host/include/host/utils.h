#pragma once

#include <cuda_runtime.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

py::array to_numpy(
    uintptr_t           device_ptr, 
    std::vector<size_t> shape
) {
    size_t n_elements = 1;
    for (auto s : shape) n_elements *= s;

    auto result = py::array_t<float>(shape);
    auto buf = result.request();
    cudaMemcpy(buf.ptr, reinterpret_cast<void*>(device_ptr),
        n_elements * sizeof(float), cudaMemcpyDeviceToHost);
    return result;
}

