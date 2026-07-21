#pragma once

#include <cuda_runtime.h>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "core/include/core.h"

// ============================================================================
// DEVICE COLOR INDEX UTIL
// ============================================================================

struct ColorIndex { 
    int r, g, b; 

    __device__ ColorIndex(
        size_t C,
        size_t H,
        size_t W
    ) {
        ProcessIndex p_idx = get_process_index();
        r = p_idx.z * (C * H * W) + 0 * (H * W) + p_idx.y * W + p_idx.x;
        g = p_idx.z * (C * H * W) + 1 * (H * W) + p_idx.y * W + p_idx.x; 
        b = p_idx.z * (C * H * W) + 2 * (H * W) + p_idx.y * W + p_idx.x;
    }

    __device__ Color get_color(float* ptr) {
        return Color(ptr[r], ptr[g], ptr[b]);
    }

    __device__ void set_color(float* ptr, const Color& c) {
        ptr[r] = c.r(); ptr[g] = c.g(); ptr[b] = c.b();
    }

    __device__ void set_color(
        uint8_t* ptr, 
        const uint8_t new_r,
        const uint8_t new_g,
        const uint8_t new_b
    ) {
        ptr[r] = new_r; ptr[g] = new_g; ptr[b] = new_b;
    }
};

// ============================================================================
// DATAVIEW
// ============================================================================

struct DataView {
public:

    float* ptr;
    size_t C, H, W;
    bool   enabled;

    /* CONSTRUCTORS */

    __host__ DataView() : ptr(nullptr), C(0), H(0), W(0), enabled(false) { }

    __host__ DataView(
        float* data_ptr,
        const size_t channels,
        const size_t height,
        const size_t width,
        const bool is_enabled
    ) : ptr(data_ptr), 
        C(channels),
        H(height), 
        W(width), 
        enabled(is_enabled)
    { }

    /* INSPECTION UTILITIES */

    __device__ size_t n_pixels()   const { return H * W; }
    __device__ size_t n_elements() const { return C * n_pixels(); }
    __device__ size_t n_bytes()    const { 
        return n_elements() * sizeof(float); 
    }

    /* INDEX UTILITIES */

    __device__ ColorIndex get_color_index() { return ColorIndex(C, H, W); }

    /* GETTERS / SETTERS */

    __device__ Color get_color() { 
        return ColorIndex(C, H, W).get_color(ptr); 
    }

    __device__ void set_color(const Color& c) {
        ColorIndex(C, H, W).set_color(ptr, c);
    }

    __device__ void set_color(const float r, const float g, const float b) {
        set_color(Color(r, g, b));
    }

    /* OPERATORS */

    __device__ DataView& operator+=(const Color& c) {
        ColorIndex idx(C, H, W);
        idx.set_color(ptr, idx.get_color(ptr) + c);
        return *this;
    }
    
    __device__ DataView& operator+=(const float v) { 
        *this += Color(v); 
        return *this;
    }

    __device__ DataView& operator*=(const Color& c) {
        ColorIndex idx(C, H, W);
        idx.set_color(ptr, idx.get_color(ptr) * c);
        return *this;
    }

    __device__ DataView& operator*=(const float v) { 
        *this *= Color(v);
        return *this;
    }

};

void run_combine_data(DataView a, DataView b);
void run_norm_by_samples(DataView data, uint32_t samples);
void run_accumulate_samples(DataView a, DataView b,uint32_t current_sample);
void run_replace_invalid(DataView data);
void run_linear_to_gamma(DataView data);
void run_tonemap(DataView data, float exposure);
void to_image(DataView data, uint8_t* result);

// ============================================================================
// DATA OBJECT CLASS
// ============================================================================

class DataObject {
public:

    size_t C, H, W; // Channels, Height, Width

    __host__ ~DataObject() { if (data_ptr()) cudaFree(data_ptr()); }

    /* CONSTRUCTORS */

    __host__ DataObject() : C(0), H(0), W(0), enabled(false) {}

    __host__ DataObject(
        size_t n_channels, 
        size_t h, 
        size_t w
    ) : C(n_channels), H(h), W(w) {
        cudaMalloc(&d_ptr, n_bytes());
        cudaMemset(d_ptr, 0, n_bytes());
    }

    __host__ DataObject(DataObject* data) 
        : DataObject(data->C, data->H, data->W) { }

    DataObject(DataObject&& other) noexcept {
        C       = other.C;
        H       = other.H;
        W       = other.W;
        enabled = other.enabled;
        d_ptr   = other.d_ptr;

        other.d_ptr = nullptr;
    }

    DataObject& operator=(DataObject&& other) noexcept {
        if (this != &other) {
            C = other.C; H = other.H; W = other.W;
            enabled     = other.enabled;
            d_ptr       = other.d_ptr;
            other.d_ptr = nullptr;
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
    
    /* POINTER REFERENCES */

    __host__ __device__ float* data_ptr() { return d_ptr; }
    __host__ __device__ uintptr_t device_ptr() { 
        return reinterpret_cast<uintptr_t>(d_ptr); 
    }

    /* KERNEL UTILITIES */

    void combine(DataObject& other) {
        if (enabled && other.enabled) run_combine_data(view(), other.view()); 
    }

    void normalize_by_samples(uint32_t samples) {
        if (enabled) run_norm_by_samples(view(), samples);
    }

    void accumulate_samples(DataObject& other, uint32_t current_sample) {
        if (enabled && other.enabled) {
            run_accumulate_samples(view(), other.view(), current_sample); 
        }
    }

    void replace_invalid() { if (enabled) run_replace_invalid(view()); }
    void linear_to_gamma() { if (enabled) run_linear_to_gamma(view()); }
    void tonemap(float exposure) {if (enabled) run_tonemap(view(), exposure);}

    /* HOST UTILITIES */

    __host__ DataView view() { 
        return DataView{ data_ptr(), C, H, W, enabled}; 
    }

    __host__ pybind11::array numpy() {
        cudaDeviceSynchronize();

        uint8_t* image_ptr;
        cudaMalloc(&image_ptr, n_elements() * sizeof(uint8_t));
        to_image(view(), image_ptr);

        auto result = pybind11::array_t<uint8_t>(shape());
        cudaMemcpy(
            result.request().ptr, image_ptr, 
            n_elements() * sizeof(uint8_t), 
            cudaMemcpyDeviceToHost
        );
        cudaFree(image_ptr);

        return result;
    }

private:

    bool enabled = true;
    float* d_ptr = nullptr;

};

// ============================================================================
// TRANSFER STREAM
// ============================================================================

class TransferStream {
public:

    /* CONSTRUCTORS */

    __host__ ~TransferStream() { destroy(); }

    __host__ TransferStream() : data(nullptr), C(0), H(0), W(0) { }

    /* LINKING */

    __host__ void link(DataObject* obj) {
        if (enabled) { destroy(); enabled = false; }
        C = obj->C; H = obj->H; W = obj->W;
        data = obj;
    }

    __host__ void unlink(DataObject* obj) { link(nullptr); }
    __host__ bool is_linked() const { return data != nullptr; }

    /* STREAM CONTROL */

    __host__ void start() {
        if (enabled) destroy();
        cudaMallocHost(&stream_buffer, n_bytes());
        cudaMalloc(&image_buffer, n_bytes());
        cudaStreamCreate(&transfer_stream);
        enabled = true;
    }

    __host__ void destroy() {
        if (!enabled) return;
        destroy_buffer(stream_buffer);
        destroy_buffer(image_buffer);
        if (transfer_stream) cudaStreamDestroy(transfer_stream);
        enabled = false;
    }

    /* DATA ACCESS */

    __host__ uintptr_t buffer_ptr() {
        return reinterpret_cast<uintptr_t>(stream_buffer);
    }

    __host__ uintptr_t readback() {
        to_image(data->view(), image_buffer);

        cudaMemcpyAsync(
            stream_buffer, image_buffer, n_bytes(),
            cudaMemcpyDeviceToHost, transfer_stream
        );

        cudaStreamSynchronize(transfer_stream);
        return buffer_ptr();
    }

    __host__ std::array<size_t, 3> shape() { return { C, H, W }; }
    __host__ size_t n_pixels()   const { return data->n_pixels(); }
    __host__ size_t n_elements() const { return data->n_elements(); }
    __host__ size_t n_bytes()    const { 
        return n_elements() * sizeof(uint8_t); 
    }

private:

    size_t C, H, W;
    bool enabled = false;
    DataObject* data = nullptr;

    uint8_t* stream_buffer = nullptr;
    uint8_t* image_buffer  = nullptr;
    cudaStream_t transfer_stream;

    __host__ bool destroy_buffer(uint8_t* buffer) {
        if (!buffer) return false;
        cudaFreeHost(buffer); buffer = nullptr;
        return true;
    }

};

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

    __host__ void replace_invalid_values() {
        for (DataObject* obj : get_data())
            if (obj->is_enabled()) obj->replace_invalid();
    }

    __host__ void normalize_by_samples(uint32_t total_samples) {
        for (DataObject* obj : get_data())
            if (obj->is_enabled()) obj->normalize_by_samples(total_samples);
    }

    __host__ DataObject* get_layer(LayerType layer_type) {
        switch (layer_type) {
        case LayerType::BEAUTY:    return &beauty;
        case LayerType::DIFFUSE:   return &diffuse;
        case LayerType::SPECULAR:  return &specular;
        case LayerType::NORMAL:    return &normal;
        case LayerType::SHADOW:    return &shadow;
        case LayerType::DEPTH:     return &depth;
        case LayerType::EMISSION:  return &emission;
        case LayerType::OBJECT_ID: return &object_id;
        default:
            throw std::runtime_error(
                "DataObject::get_layer() encountered invalid LayerType.");
        }
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


