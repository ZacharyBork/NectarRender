#include "engine/include/engine/data.h"

// ============================================================================
// UTILITIES
// ============================================================================

__global__ void linear_to_gamma_kernel(DataObject data) {
    ColorIndex c_idx = ColorIndex::from_process(data.C, data.H, data.W);

    float* data_ptr = data.data_ptr();
    Color curr = c_idx.get_color(data_ptr);
    c_idx.set_color(data_ptr, Color(
        curr.r() > 0.0f ? sqrtf(curr.r()) : 0.0f,
        curr.g() > 0.0f ? sqrtf(curr.g()) : 0.0f,
        curr.b() > 0.0f ? sqrtf(curr.b()) : 0.0f
    ));
};

void run_linear_to_gamma(DataObject data) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    linear_to_gamma_kernel<<<grid, block>>>(data);
}

// ============================================================================
// COMBINATION
// ============================================================================

__global__ void combine_data_kernel(DataObject a, DataObject b) {
    ColorIndex c_idx = ColorIndex::from_process(a.C, a.H, a.W);
    float* a_data = a.data_ptr();
    Color a_color = c_idx.get_color(a_data);
    c_idx.set_color(a_data, a_color + c_idx.get_color(b.data_ptr()));
}

void run_combine_data(DataObject a, DataObject b) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((a.W + BS2D - 1) / BS2D, (a.H + BS2D - 1) / BS2D, 1);
    combine_data_kernel<<<grid, block>>>(a, b);
}

// ============================================================================
// NORMALIZATION
// ============================================================================

__global__ void norm_by_samples_kernel(
    DataObject   data, 
    unsigned int samples
) {
    ColorIndex c_idx = ColorIndex::from_process(data.C, data.H, data.W);
    float* data_ptr = data.data_ptr();
    Color c = c_idx.get_color(data_ptr) * (1.0 / (float)samples);
    c_idx.set_color(data_ptr, c);
}

void run_norm_by_samples(
    DataObject   data, 
    unsigned int samples
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    norm_by_samples_kernel<<<grid, block>>>(data, samples);
}

// ============================================================================
// COLOR CORRECTION
// ============================================================================

__global__ void tonemap_reinhard_kernel(DataObject data, float exposure) {
    ColorIndex c_idx = ColorIndex::from_process(data.C, data.H, data.W);
    float* data_ptr = data.data_ptr();
    Color c = c_idx.get_color(data_ptr);
    c_idx.set_color(data_ptr, c * (1.0f / (c + Color::white())));
}

void run_tonemap(
    DataObject data, 
    float exposure
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    tonemap_reinhard_kernel<<<grid, block>>>(data, exposure);
}


