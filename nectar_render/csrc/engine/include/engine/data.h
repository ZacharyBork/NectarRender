#pragma once

#include <vector>
#include <stdint.h>
#include <cuda_runtime.h>

#include "core/include/core/utils.h"
#include "core/include/core/vector.h"

// ============================================================================
// UTILITIES
// ============================================================================

struct ColorIndex { 
    int r, g, b; 

    __host__ __device__ ColorIndex(int r, int g, int b) : r(r), g(g), b(b) { }
};

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

    DataObject(std::vector<size_t> shape) {
        C = shape[0]; H = shape[1]; W = shape[2];
        n_elements = C * H * W;
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

    __host__ __device__ bool is_enabled() { return enabled; }
    __host__ __device__ float* data_ptr() { 
        return reinterpret_cast<float*>(device_ptr); 
    }

    __host__ void combine(DataObject other);
    __host__ void normalize_by_samples(unsigned int samples);

    __device__ void set_color(int x, int y, int z, Color color) {
        int r = z * (C * H * W) + 0 * (H * W) + y * W + x;
        int g = z * (C * H * W) + 1 * (H * W) + y * W + x;
        int b = z * (C * H * W) + 2 * (H * W) + y * W + x;

        float* d_ptr = data_ptr();
        d_ptr[r] = color.x();
        d_ptr[g] = color.y();
        d_ptr[b] = color.z();
    }

};

void combine_data(DataObject a, DataObject b);
void norm_by_samples(DataObject data, unsigned int samples);

inline void DataObject::combine(DataObject other) { 
    combine_data(*this, other); 
}

inline void DataObject::normalize_by_samples(unsigned int samples) {
    norm_by_samples(*this, samples);
}

// ============================================================================
// RENDER LAYERS CLASS
// ============================================================================

struct RenderLayersConfig {
    const bool beauty    = true;
    const bool diffuse   = false;
    const bool specular  = false;
    const bool normal    = false;
    const bool shadow    = false;
    const bool depth     = false;
    const bool emission  = false;
    const bool object_id = false;
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

    RenderLayers(
        size_t h, 
        size_t w,
        RenderLayersConfig cfg = {}
    ) : H(h), W(w) {
        build_layers(cfg);
    }

    RenderLayers(
        std::vector<size_t> shape,
        RenderLayersConfig cfg = {}
    ) : H(shape[1]), W(shape[2]) {
        build_layers(cfg);
    }

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
        std::vector<DataObject> objects = get_data();
        for (DataObject object : objects) 
            object.normalize_by_samples(samples);
    }

private:

    __host__ DataObject construct_data_object(size_t channels) {
        return DataObject(channels, H, W);
    }

    __host__ void build_layers(RenderLayersConfig cfg) {
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


