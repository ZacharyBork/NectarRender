#include "engine/include/engine/data.h"

// ============================================================================
// UTILITIES
// ============================================================================

__device__ ColorIndex get_color_index(
    ProcessIndex p_idx, 
    size_t C, 
    size_t H, 
    size_t W
) {
    int r = p_idx.z * (C * H * W) + 0 * (H * W) + p_idx.y * W + p_idx.x;
    int g = p_idx.z * (C * H * W) + 1 * (H * W) + p_idx.y * W + p_idx.x;
    int b = p_idx.z * (C * H * W) + 2 * (H * W) + p_idx.y * W + p_idx.x;

    return ColorIndex(r, g, b);
}

__global__ void linear_to_gamma_kernel(DataObject data) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= data.W || p_idx.y >= data.H) return;
    ColorIndex c_idx = get_color_index(p_idx, data.C, data.H, data.W);

    float* data_ptr = data.data_ptr();
    ColorValues curr(data_ptr, c_idx);

    data_ptr[c_idx.r] *= curr.r > 0.0f ? sqrtf(curr.r) : 0.0f;
    data_ptr[c_idx.g] *= curr.g > 0.0f ? sqrtf(curr.g) : 0.0f;
    data_ptr[c_idx.b] *= curr.b > 0.0f ? sqrtf(curr.b) : 0.0f;
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
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= a.W || p_idx.y >= a.H) return;
    ColorIndex c_idx = get_color_index(p_idx, a.C, a.H, a.W);

    float* a_data = a.data_ptr();
    float* b_data = b.data_ptr();

    a_data[c_idx.r] += b_data[c_idx.r];
    a_data[c_idx.g] += b_data[c_idx.g];
    a_data[c_idx.b] += b_data[c_idx.b];
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
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= data.W || p_idx.y >= data.H) return;
    ColorIndex c_idx = get_color_index(p_idx, data.C, data.H, data.W);

    float* data_ptr = data.data_ptr();
    float scale = 1.0 / (float)samples;

    data_ptr[c_idx.r] *= scale;
    data_ptr[c_idx.g] *= scale;
    data_ptr[c_idx.b] *= scale;
}

void run_norm_by_samples(
    DataObject   data, 
    unsigned int samples
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    norm_by_samples_kernel<<<grid, block>>>(data, samples);
}



