#include "engine/include/engine/data.h"

// ============================================================================
// UTILITIES
// ============================================================================

__global__ void linear_to_gamma_kernel(DataView data) {
    ColorIndex c_idx = ColorIndex::from_process(data.C, data.H, data.W);

    float* data_ptr = data.ptr;
    Color curr = c_idx.get_color(data_ptr);
    c_idx.set_color(data_ptr, Color(
        curr.r() > 0.0f ? sqrtf(curr.r()) : 0.0f,
        curr.g() > 0.0f ? sqrtf(curr.g()) : 0.0f,
        curr.b() > 0.0f ? sqrtf(curr.b()) : 0.0f
    ));
};

void run_linear_to_gamma(DataView data) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    linear_to_gamma_kernel<<<grid, block>>>(data);
}

// ============================================================================
// COMBINATION
// ============================================================================

__global__ void combine_data_kernel(DataView a, DataView b) {
    ColorIndex c_idx = ColorIndex::from_process(a.C, a.H, a.W);
    float* a_data = a.ptr;
    Color a_color = c_idx.get_color(a_data);
    c_idx.set_color(a_data, a_color + c_idx.get_color(b.ptr));
}

void run_combine_data(DataView a, DataView b) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((a.W + BS2D - 1) / BS2D, (a.H + BS2D - 1) / BS2D, 1);
    combine_data_kernel<<<grid, block>>>(a, b);
}

__global__ void accumulate_samples_kernel(
    DataView a, 
    DataView b,
    uint32_t current_sample
) {
    ColorIndex c_idx = ColorIndex::from_process(a.C, a.H, a.W);
    
    Color avg    = c_idx.get_color(a.ptr);
    Color sample = c_idx.get_color(b.ptr);

    float fn = static_cast<float>(current_sample);
    Color result = avg * ((fn - 1.0f) / fn) + sample * (1.0f / fn);
    
    c_idx.set_color(a.ptr, result);
}

void run_accumulate_samples(
    DataView a, 
    DataView b,
    uint32_t current_sample
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((a.W + BS2D - 1) / BS2D, (a.H + BS2D - 1) / BS2D, 1);
    accumulate_samples_kernel<<<grid, block>>>(a, b, current_sample);
}

// ============================================================================
// NORMALIZATION
// ============================================================================

__global__ void norm_by_samples_kernel(
    DataView data, 
    uint32_t samples
) {
    ColorIndex c_idx = ColorIndex::from_process(data.C, data.H, data.W);
    float* data_ptr = data.ptr;
    Color c = c_idx.get_color(data_ptr) * (1.0 / (float)samples);
    c_idx.set_color(data_ptr, c);
}

void run_norm_by_samples(
    DataView   data, 
    uint32_t samples
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    norm_by_samples_kernel<<<grid, block>>>(data, samples);
}

// ============================================================================
// COLOR CORRECTION
// ============================================================================

__global__ void tonemap_reinhard_kernel(DataView data) {
    ColorIndex c_idx = ColorIndex::from_process(data.C, data.H, data.W);
    float* data_ptr = data.ptr;
    Color c = c_idx.get_color(data_ptr);
    c_idx.set_color(data_ptr, c / (c + 1.0f));
}

void run_tonemap(
    DataView data, 
    float exposure
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    tonemap_reinhard_kernel<<<grid, block>>>(data);
}


