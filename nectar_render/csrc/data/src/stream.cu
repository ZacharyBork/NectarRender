#include "data/include/data/stream.h"

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


