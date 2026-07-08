#include "engine/include/engine/data.h"

// ============================================================================
// UTILITIES
// ============================================================================

__global__ void linear_to_gamma_kernel(DataView data) {

    Color curr = data.get_color();
    data.set_color(Color(
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
    Color a_color = a.get_color();
    a.set_color(a_color + b.get_color());
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
    Color avg    = a.get_color();
    Color sample = b.get_color();

    float fn = static_cast<float>(current_sample);
    Color result = avg * ((fn - 1.0f) / fn) + sample * (1.0f / fn);
    
    a.set_color(result);
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
// ERROR CORRECTION
// ============================================================================

__global__ void replace_invalid_kernel(DataView data) {
    Color c = data.get_color();
    for (int channel = 0; channel < data.C; channel++)
        if (isnan(c[channel])) c[channel] = 0.0f;
    data.set_color(c);
}

void run_replace_invalid(DataView data) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    replace_invalid_kernel<<<grid, block>>>(data);
}

// ============================================================================
// NORMALIZATION
// ============================================================================

__global__ void norm_by_samples_kernel(
    DataView data, 
    uint32_t samples
) {
    Color c = data.get_color() * (1.0f / (float)samples);
    data.set_color(c);
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
    Color c = data.get_color();
    data.set_color(c / (c + 1.0f));
}

void run_tonemap(
    DataView data, 
    float exposure
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    tonemap_reinhard_kernel<<<grid, block>>>(data);
}

// ============================================================================
// IMAGE CONVERSION
// ============================================================================

__global__ void to_image_kernel(DataView data, uint8_t* result) {
    ColorIndex idx = data.get_color_index();
    Color c = data.get_color();

    for (int channel = 0; channel < data.C; channel++) {
        if (isnan(c[channel])) c[channel] = 0.0f;
        c[channel] = fmaxf(0.0f, fminf(1.0f, c[channel]));
        c[channel] *= 255.0f;
    }

    idx.set_color(result,
        static_cast<uint8_t>(c.r()),
        static_cast<uint8_t>(c.g()),
        static_cast<uint8_t>(c.b())
    );
}

void to_image(DataView data, uint8_t* result) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    to_image_kernel<<<grid, block>>>(data, result);
}

