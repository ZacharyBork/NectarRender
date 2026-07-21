#pragma once

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

