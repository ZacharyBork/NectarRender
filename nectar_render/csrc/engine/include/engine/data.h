#pragma once

#include <stdint.h>
#include <cuda_runtime.h>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "core/include/core.h"

// ============================================================================
// DATA OBJECT CLASS
// ============================================================================

struct DataView {
    float*  ptr;
    size_t  C, H, W;
    bool    enabled;

    __host__ __device__ size_t n_pixels()   const { return H * W; }
    __host__ __device__ size_t n_elements() const { return C * n_pixels(); }
    __host__ __device__ size_t n_bytes()    const { 
        return n_elements() * sizeof(float); 
    }

    __device__ void set_color(Color color) {
        ColorIndex idx = ColorIndex::from_process(C, H, W);
        idx.set_color(ptr, color);
    }
    
};

class DataObject {
public:

    size_t C, H, W; // Channels, Height, Width

    __host__ ~DataObject() { if(is_pinned()) free_pinned_buffer(); }

    /* CONSTRUCTORS */

    __host__ DataObject() : C(0), H(0), W(0), enabled(false) {}

    __host__ DataObject(
        size_t n_channels, 
        size_t h, 
        size_t w
    ) : C(n_channels), H(h), W(w) {
        d_ptr = allocate_cuda_memory(n_elements(), 0.0f);
    }

    __host__ DataObject(
        DataObject* data
    ) : DataObject(data->C, data->H, data->W) { }

    DataObject(DataObject&& other) noexcept {
        C       = other.C;
        H       = other.H;
        W       = other.W;
        enabled = other.enabled;
        d_ptr   = other.d_ptr;
        pinned_buffer   = other.pinned_buffer;
        transfer_stream = other.transfer_stream;

        other.d_ptr = 0;
        other.pinned_buffer = nullptr;
    }

    DataObject& operator=(DataObject&& other) noexcept {
        if (this != &other) {
            free_pinned_buffer();
            C = other.C; H = other.H; W = other.W;
            enabled         = other.enabled;
            d_ptr           = other.d_ptr;
            pinned_buffer   = other.pinned_buffer;
            transfer_stream = other.transfer_stream;
            other.d_ptr         = 0;
            other.pinned_buffer = nullptr;
        }
        return *this;
    }

    DataObject(const DataObject&)            = delete;
    DataObject& operator=(const DataObject&) = delete;

    /* SHAPE / SIZE UTILS */

    __host__ const std::vector<size_t> shape() const {
        return { C, H, W };
    }

    __host__ __device__ size_t n_pixels()   const { return H * W; }
    __host__ __device__ size_t n_elements() const { return C * n_pixels(); }
    __host__ __device__ size_t n_bytes()    const { 
        return n_elements() * sizeof(float); 
    }

    /* STATE CHECKERS */

    __host__ __device__ bool is_enabled() { return enabled; }
    __host__ __device__ bool is_pinned()  { return pinned_buffer != nullptr; }
    
    /* POINTER REFERENCES */

    __host__ __device__ uintptr_t device_ptr() { return d_ptr; }
    __host__ __device__ float* data_ptr() { 
        return reinterpret_cast<float*>(d_ptr);
    }

    /* PINNED BUFFERS */

    __host__ void pin_buffer() {
        if (pinned_buffer) return;
        cudaMallocHost(&pinned_buffer, n_bytes());
        cudaStreamCreate(&transfer_stream);
    }

    __host__ uintptr_t readback_pinned() {
        if (!pinned_buffer)
            throw std::runtime_error(
                "DataObject::readback_pinned() is only valid for DataObjects "
                "with pinned buffers. For unpinned DataObjects, please use "
                "DataObject::device_ptr() instead."
            );

        cudaMemcpyAsync(
            pinned_buffer,
            reinterpret_cast<void*>(d_ptr),
            n_bytes(),
            cudaMemcpyDeviceToHost,
            transfer_stream
        );
        cudaStreamSynchronize(transfer_stream);
        return reinterpret_cast<uintptr_t>(pinned_buffer);
    }

    /* KERNEL WRAPPERS */

    __host__ void combine(DataObject& other);
    __host__ void accumulate_samples(
        DataObject& other,
        uint32_t current_sample
    );

    __host__ void normalize_by_samples(uint32_t samples);
    __host__ void linear_to_gamma();
    __host__ void tonemap(float exposure);

    /* HOST UTILITIES */

    __host__ DataView view() { 
        return DataView{ data_ptr(), C, H, W, enabled}; 
    }

    __host__ pybind11::array numpy() {
        if (pinned_buffer != nullptr)
            throw std::runtime_error(
                "DataObject::numpy() is only valid for DataObjects with "
                "unpinned buffers. For pinned DataObjects, please use "
                "DataObject::readback_pinned() instead."
            );

        cuda_synchronize();

        auto result = pybind11::array_t<float>(shape());
        auto buf = result.request();
        
        cudaDeviceSynchronize();
        cudaMemcpy(buf.ptr, reinterpret_cast<void*>(d_ptr),
            n_bytes(), cudaMemcpyDeviceToHost);

        return result;
    }

private:

    bool enabled = true;
    uintptr_t d_ptr = 0;
    float* pinned_buffer = nullptr;
    cudaStream_t transfer_stream;

    __host__ void free_pinned_buffer() {
        if (pinned_buffer) {
            cudaFreeHost(pinned_buffer);
            pinned_buffer = nullptr;
            cudaStreamDestroy(transfer_stream);
        }
    }

};

void run_combine_data(DataView a, DataView b);
void run_accumulate_samples(DataView a, DataView b,uint32_t current_sample);

void run_norm_by_samples(DataView data, uint32_t samples);
void run_linear_to_gamma(DataView data);
void run_tonemap(DataView data, float exposure);

inline void DataObject::combine(DataObject& other) {
    if (this->enabled && other.enabled) 
        run_combine_data(this->view(), other.view()); 
}

inline void DataObject::accumulate_samples(
    DataObject& other,
    uint32_t current_sample
) {
    if (this->enabled && other.enabled) {
        run_accumulate_samples(this->view(), other.view(), current_sample); 
    }
}

inline void DataObject::normalize_by_samples(uint32_t samples) {
    if (this->enabled) run_norm_by_samples(this->view(), samples);
}

inline void DataObject::linear_to_gamma() { 
    if (this->enabled) run_linear_to_gamma(this->view()); 
}

inline void DataObject::tonemap(float exposure) { 
    if (this->enabled) run_tonemap(this->view(), exposure); 
}

// ============================================================================
// RENDER LAYERS CLASS
// ============================================================================

const inline uint8_t N_RENDER_LAYERS = 8;

enum class LayerType {
    BEAUTY, DIFFUSE, SPECULAR, NORMAL, SHADOW, DEPTH, EMISSION, OBJECT_ID
};

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

struct AOVs {
    size_t H, W;
    DataView beauty, diffuse, specular, normal, 
             shadow, depth, emission, object_id;

    __host__ AOVs(
        size_t H, size_t W,
        DataObject& beauty,
        DataObject& diffuse,
        DataObject& specular,
        DataObject& normal,
        DataObject& shadow,
        DataObject& depth,
        DataObject& emission,
        DataObject& object_id
    ) : H(H), W(W),
        beauty(beauty.view()), 
        diffuse(diffuse.view()), 
        specular(specular.view()),
        normal(normal.view()), 
        shadow(shadow.view()), 
        depth(depth.view()), 
        emission(emission.view()),
        object_id(object_id.view()) 
    { }
};

class RenderLayers {
public:

    size_t H, W;
    RenderLayersConfig cfg;

    DataObject beauty;
    DataObject diffuse;
    DataObject specular;
    DataObject normal;
    DataObject shadow;
    DataObject depth;
    DataObject emission;
    DataObject object_id;

    __host__ RenderLayers() : H((size_t)0), W((size_t)0), cfg({}) { }

    __host__ RenderLayers(
        size_t h, 
        size_t w,
        const RenderLayersConfig& cfg = {}
    ) : H((size_t)h), W((size_t)w), cfg(cfg) { 
        build_layers(); 
    }

    __host__ RenderLayers(
        const Vector2& res,
        const RenderLayersConfig& cfg = {}
    ) : RenderLayers(res[0], res[1], cfg) { }

    __host__ RenderLayers(
        RenderLayers* t,
        const RenderLayersConfig& cfg = {}
    ) : RenderLayers(t->H, t->W, t->cfg) { }

    __host__ AOVs* aovs() {
        AOVs aovs_obj(
            H, W, beauty, diffuse, specular, normal, 
            shadow, depth, emission, object_id
        );

        AOVs* d_aov_ptr;
        size_t n_bytes = sizeof(aovs_obj);
        cudaMalloc(&d_aov_ptr, n_bytes);
        cudaMemcpy(
            reinterpret_cast<void*>(d_aov_ptr), 
            &aovs_obj, n_bytes, cudaMemcpyHostToDevice);

        return d_aov_ptr;

    }

    __host__ std::array<DataObject*, N_RENDER_LAYERS> get_data() {
        return {
            &beauty, &diffuse, &specular, &normal,
            &shadow, &depth, &emission, &object_id
        };
    }

    __host__ void combine(RenderLayers& other) { 
        std::array<DataObject*, N_RENDER_LAYERS> this_data = get_data();
        std::array<DataObject*, N_RENDER_LAYERS> other_data = other.get_data();
        for (int i = 0; i < N_RENDER_LAYERS; i++) {
            this_data[i]->combine(*other_data[i]);
        }    
    }

    __host__ void accumulate(
        RenderLayers& other,
        uint32_t current_sample
    ) {
        std::array<DataObject*, N_RENDER_LAYERS> this_data = get_data();
        std::array<DataObject*, N_RENDER_LAYERS> other_data = other.get_data();
        for (int i = 0; i < N_RENDER_LAYERS; i++) {
            this_data[i]->accumulate_samples(*other_data[i], current_sample);
        }
    }

    __host__ void clear() {
        for (DataObject* obj : get_data())
            if (obj->is_enabled())
                cudaMemset(obj->data_ptr(), 0, obj->n_bytes());
    }

    __host__ void normalize_by_samples(uint32_t total_samples) {
        for (DataObject* obj : get_data())
            if (obj->is_enabled()) obj->normalize_by_samples(total_samples);
    }

    __host__ DataObject* get_layer(LayerType layer_type) {
        switch (layer_type) {
        case LayerType::BEAUTY:     return &beauty;
        case LayerType::DIFFUSE:    return &diffuse;
        case LayerType::SPECULAR:   return &specular;
        case LayerType::NORMAL:     return &normal;
        case LayerType::SHADOW:     return &shadow;
        case LayerType::DEPTH:      return &depth;
        case LayerType::EMISSION:   return &emission;
        case LayerType::OBJECT_ID:  return &object_id;
        default:
            throw std::runtime_error(
                "DataObject::get_layer() encountered invalid LayerType.");
        }
    }

    __host__ void pin_buffer(LayerType layer_type) {
        DataObject* layer = get_layer(layer_type);
        if (layer->is_enabled()) layer->pin_buffer();
        else throw std::runtime_error(
            "DataObject pin_layer called on disabled layer.");
    }

private:

    __host__ DataObject construct_data_object(size_t channels) {
        return DataObject(channels, H, W);
    }

    __host__ void build_layers() {
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


