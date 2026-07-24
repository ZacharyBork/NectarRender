#include "data/include/data/stream.h"

// ============================================================================
// TONEMAPPING
// ============================================================================

__device__ void tonemap_reinhard(Color& pixel_color) {
    Color temp = Color(pixel_color.r(), pixel_color.g(), pixel_color.b());
    temp /= temp + 1.0f;
    
}

__device__ void tonemap_reinhard_extended(
    Color& pixel_color, 
    float white_point
) {
    float inv_white2 = 1.0f / (white_point * white_point);
    Color numerator = pixel_color * (Color(1.0f) + pixel_color * inv_white2);
    pixel_color = numerator / (Color(1.0f) + pixel_color);
}

__device__ void tonemap_aces(Color& pixel_color) {
    const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    Color x = Color(pixel_color.r(), pixel_color.g(), pixel_color.b());
    Color numerator   = x * (x * a + b);
    Color denominator = x * (x * c + d) + e;
    pixel_color = Color(
        fminf(1.0f, fmaxf(0.0f, numerator.r() / denominator.r())),
        fminf(1.0f, fmaxf(0.0f, numerator.g() / denominator.g())),
        fminf(1.0f, fmaxf(0.0f, numerator.b() / denominator.b()))
    );
}

__device__ void apply_tonemapping(
    Color& pixel_color,
    StreamConfig cfg
) {
    Color col = Color(pixel_color.r(), pixel_color.g(), pixel_color.b());
    
    switch (cfg.tm_method) {
        case TonemapMethod::REINHARD: 
            tonemap_reinhard(col);
            break;
        case TonemapMethod::REINHARD_EXTENDED: 
            tonemap_reinhard_extended(col, cfg.tm_white_point); 
            break;
        case TonemapMethod::ACES:
            tonemap_aces(col); 
            break;
    }

    float alpha = cfg.tm_alpha;
    pixel_color = ((1.0f - alpha) * pixel_color + alpha * col);
    pixel_color = Color(
        fminf(1.0f, fmaxf(0.0f, pixel_color.r())),
        fminf(1.0f, fmaxf(0.0f, pixel_color.g())),
        fminf(1.0f, fmaxf(0.0f, pixel_color.b()))
    );
}

// ============================================================================
// DATA UTILS
// ============================================================================

__device__ void linear_to_gamma(Color& pixel_color) {
    pixel_color = Color(
        pixel_color.r() > 0.0f ? sqrtf(pixel_color.r()) : 0.0f,
        pixel_color.g() > 0.0f ? sqrtf(pixel_color.g()) : 0.0f,
        pixel_color.b() > 0.0f ? sqrtf(pixel_color.b()) : 0.0f
    );
};

__device__ void to_image(
    size_t C,
    Color& pixel_color, 
    ColorIndex& c_idx,
    uint8_t* result
) {
    for (int channel = 0; channel < C; channel++) {
        if (isnan(pixel_color[channel])) pixel_color[channel] = 0.0f;
        pixel_color[channel] = fmaxf(0.0f, fminf(1.0f, pixel_color[channel]));
        pixel_color[channel] *= 255.0f;
    }

    c_idx.set_color(result,
        static_cast<uint8_t>(pixel_color.r()),
        static_cast<uint8_t>(pixel_color.g()),
        static_cast<uint8_t>(pixel_color.b())
    );
}

// ============================================================================
// STREAM PROCESSING
// ============================================================================

__global__ void process_stream_kernel(
    DataView data, 
    uint8_t* result, 
    StreamConfig cfg
) {
    ColorIndex c_idx(data.C, data.H, data.W);
    Color pixel_color = c_idx.get_color(data.ptr);
    
    if (cfg.apply_tonemapping) apply_tonemapping(pixel_color, cfg);
    if (cfg.linear_to_gamma)   linear_to_gamma(pixel_color);
    
    to_image(data.C, pixel_color, c_idx, result);
}

void process_stream(DataView data, uint8_t* result, StreamConfig cfg) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    process_stream_kernel<<<grid, block>>>(data, result, cfg);
        
}

// ============================================================================
// OVERLAY
// ============================================================================

__global__ void composite_overlay_kernel(
    uint8_t* data,
    uint8_t* mask,
    Color    color,
    size_t  C, size_t  H, size_t  W,
    uint8_t r, uint8_t g, uint8_t b
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= W || p_idx.y >= H) return;
    uint32_t idx = p_idx.y * W + p_idx.x;

    uint32_t alpha = mask[idx];
    if (alpha == 0u) return;
    uint32_t inv_alpha = 255u - alpha;

    ColorIndex c_idx(C, H, W);
    data[c_idx.r] = static_cast<uint8_t>(
        (r * alpha + data[c_idx.r] * inv_alpha + 127u) / 255u
    );
    data[c_idx.g] = static_cast<uint8_t>(
        (g * alpha + data[c_idx.g] * inv_alpha + 127u) / 255u
    );
    data[c_idx.b] = static_cast<uint8_t>(
        (b * alpha + data[c_idx.b] * inv_alpha + 127u) / 255u
    );
}

void composite_overlay(
    uint8_t* data,
    uint8_t* mask,
    Color    color,
    size_t C, size_t H, size_t W
) {   
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((W + BS2D - 1) / BS2D, (H + BS2D - 1) / BS2D, 1);
    
    uint8_t r = static_cast<uint8_t>(
        fminf(1.0f, fmaxf(0.0f, color.r())) * 255.0f
    );
    uint8_t g = static_cast<uint8_t>(
        fminf(1.0f, fmaxf(0.0f, color.g())) * 255.0f
    );
    uint8_t b = static_cast<uint8_t>(
        fminf(1.0f, fmaxf(0.0f, color.b())) * 255.0f
    );
    
    composite_overlay_kernel<<<grid, block>>>(
        data, mask, color, C, H, W, r, g, b
    );
}


