#pragma once

#include <vector>
#include <stdint.h>
#include <cuda_runtime.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "core/include/core.h"

namespace py = pybind11;

// ============================================================================
// DATA OBJECT CLASS
// ============================================================================

class DataObject {
public:
    uintptr_t device_ptr;
    
    size_t C, H, W; // Channels, Height, Width
    size_t n_elements;

    bool enabled = true;

    DataObject() 
        : device_ptr(0), C(0), H(0), W(0), n_elements(0), enabled(false) {}

    DataObject(
        size_t n_channels, 
        size_t h, 
        size_t w
    ) : C(n_channels), H(h), W(w) {
        n_elements = n_channels * H * W;
        device_ptr = allocate_cuda_memory(n_elements, 0.0f);
    }

    DataObject(
        uintptr_t d_ptr,
        size_t    n_channels, 
        size_t    h, 
        size_t    w
    ) : device_ptr(d_ptr), C(n_channels), H(h), W(w) {
        n_elements = n_channels * H * W;
    }

    DataObject(
        DataObject* data
    ) : C(data->C), H(data->H), W(data->W), n_elements(data->n_elements) {
        device_ptr = allocate_cuda_memory(n_elements, 0.0f);
    }

    __host__ __device__ bool is_enabled() { return enabled; }
    __host__ __device__ float* data_ptr() { 
        return reinterpret_cast<float*>(device_ptr); 
    }

    __host__ void combine(DataObject other);
    __host__ void normalize_by_samples(unsigned int samples);
    __host__ void linear_to_gamma();
    __host__ void tonemap(float exposure);

    __device__ void set_color(int x, int y, int z, Color color) {
        ColorIndex idx = ColorIndex::from_process(C, H, W);
        idx.set_color(data_ptr(), color);
    }

    __host__ py::array numpy() {
        std::vector<size_t> shape = { C, H, W };
        auto result = py::array_t<float>(shape);
        auto buf = result.request();
        cudaDeviceSynchronize();
        cudaMemcpy(buf.ptr, reinterpret_cast<void*>(device_ptr),
            n_elements * sizeof(float), cudaMemcpyDeviceToHost);
        return result;
    }

};

void run_combine_data(DataObject a, DataObject b);
void run_norm_by_samples(DataObject data, unsigned int samples);
void run_linear_to_gamma(DataObject data);
void run_tonemap(DataObject data, float exposure);

inline void DataObject::combine(DataObject other) { 
    run_combine_data(*this, other); 
}

inline void DataObject::normalize_by_samples(unsigned int samples) {
    run_norm_by_samples(*this, samples);
}

inline void DataObject::linear_to_gamma() { 
    run_linear_to_gamma(*this); 
}

inline void DataObject::tonemap(float exposure) { 
    run_tonemap(*this, exposure); 
}

// ============================================================================
// RENDER LAYERS CLASS
// ============================================================================

struct RenderLayersConfig {
    bool beauty    = true;
    bool diffuse   = false;
    bool specular  = false;
    bool normal    = false;
    bool shadow    = false;
    bool depth     = false;
    bool emission  = false;
    bool object_id = false;
};

class RenderLayers {
public:

    size_t H, W;

    DataObject beauty;
    DataObject diffuse;
    DataObject specular;
    DataObject normal;
    DataObject shadow;
    DataObject depth;
    DataObject emission;
    DataObject object_id;

    __host__ RenderLayers(
        size_t h, 
        size_t w,
        const RenderLayersConfig& cfg = {}
    ) : H((size_t)h), W((size_t)w) { build_layers(cfg); }

    __host__ RenderLayers(
        const Vector2& resolution,
        const RenderLayersConfig& cfg = {}
    ) : RenderLayers(resolution[0], resolution[1], cfg) { }

    __host__ std::vector<DataObject> get_data() {
        std::vector<DataObject> objects;
        objects.push_back(beauty);
        objects.push_back(diffuse);
        objects.push_back(specular);
        objects.push_back(normal);
        objects.push_back(shadow);
        objects.push_back(depth);
        objects.push_back(emission);
        objects.push_back(object_id);
        return objects;
    }

    __host__ void normalize_by_samples(unsigned int samples) {
        for (DataObject object : get_data()) 
            object.normalize_by_samples(samples);
    }

private:

    __host__ DataObject construct_data_object(size_t channels) {
        return DataObject(channels, H, W);
    }

    __host__ void build_layers(const RenderLayersConfig& cfg) {
        if (cfg.beauty)    beauty    = construct_data_object(3);
        if (cfg.diffuse)   diffuse   = construct_data_object(3);
        if (cfg.specular)  specular  = construct_data_object(3);
        if (cfg.normal)    normal    = construct_data_object(3);
        if (cfg.shadow)    shadow    = construct_data_object(1);
        if (cfg.depth)     depth     = construct_data_object(1);
        if (cfg.emission)  emission  = construct_data_object(3);
        if (cfg.object_id) object_id = construct_data_object(3);
    }

};


