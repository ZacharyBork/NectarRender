#include "data/include/data/data_object.h"

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
// IMAGE CONVERSION
// ============================================================================

__global__ void data_to_image_kernel(DataView data, uint8_t* result) {
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

void data_to_image(DataView data, uint8_t* result) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    data_to_image_kernel<<<grid, block>>>(data, result);
}

