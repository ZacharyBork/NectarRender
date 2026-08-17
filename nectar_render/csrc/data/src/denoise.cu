#include "data/include/data/denoise.h"

// ============================================================================
// INTERLEAVING
// ============================================================================

__global__ void planar_to_interleaved_kernel(DataView data, float* out) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= data.W || p_idx.y >= data.H) return;

    Color c = data.get_color();
    uint32_t pixel_idx = p_idx.y * data.W + p_idx.x;
    out[pixel_idx * 3 + 0] = c.r();
    out[pixel_idx * 3 + 1] = c.g();
    out[pixel_idx * 3 + 2] = c.b();
}

void planar_to_interleaved(DataView data, float* out, cudaStream_t stream) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    planar_to_interleaved_kernel<<<grid, block, 0, stream>>>(data, out);
}

__global__ void interleaved_to_planar_kernel(float* in, DataView data) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= data.W || p_idx.y >= data.H) return;

    uint32_t pixel_idx = p_idx.y * data.W + p_idx.x;
    data.set_color(
        Color(in[pixel_idx*3+0], in[pixel_idx*3+1], in[pixel_idx*3+2])
    );
}

void interleaved_to_planar(float* in, DataView data, cudaStream_t stream) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    interleaved_to_planar_kernel<<<grid, block, 0, stream>>>(in, data);
}

