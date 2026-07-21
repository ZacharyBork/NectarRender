#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "data/include/data/utils.h"

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



