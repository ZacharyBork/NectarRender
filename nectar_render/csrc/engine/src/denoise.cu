#include "engine/include/engine/denoise.h"

// ############################################################################
// TV DENOISING
// ############################################################################

__global__ void tvd_compute_gradients(
    const float* u,
    float*       grad_x,
    float*       grad_y,
    int          H,
    int          W
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;

    int idx = y * W + x;

    grad_x[idx] = (x < W-1) ? u[idx + 1]   - u[idx] : 0.0f;
    grad_y[idx] = (y < H-1) ? u[idx + W]   - u[idx] : 0.0f;
}

__global__ void tvd_update(
    const float* f,
    float*       u,
    const float* grad_x,
    const float* grad_y,
    float        lambda,
    float        dt,
    int          H,
    int          W
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;

    int idx = y * W + x;

    float gx = grad_x[idx];
    float gy = grad_y[idx];
    float mag = sqrtf(gx*gx + gy*gy + EPS*EPS);

    float nx = gx / mag;
    float ny = gy / mag;

    float div_x = nx;
    float div_y = ny;

    if (x > 0) {
        int left = idx - 1;
        float gx_l = grad_x[left];
        float gy_l = grad_y[left];
        float mag_l = sqrtf(gx_l*gx_l + gy_l*gy_l + EPS*EPS);
        div_x -= gx_l / mag_l;
    }
    if (y > 0) {
        int up = idx - W;
        float gx_u = grad_x[up];
        float gy_u = grad_y[up];
        float mag_u = sqrtf(gx_u*gx_u + gy_u*gy_u + EPS*EPS);
        div_y -= gy_u / mag_u;
    }

    float divergence = div_x + div_y;
    u[idx] = u[idx] + dt * (lambda * divergence - (u[idx] - f[idx]));
}

void tv_denoise(
    DataObject& data,    
    const float lambda,
    const unsigned int iterations
) {
    const float dt  = 0.1f;

    float* d_u;
    float* d_grad_x;
    float* d_grad_y;

    int H = data.H;
    int W = data.W;

    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);

    size_t bytes = data.n_pixels() * sizeof(float);

    cudaMalloc(&d_u,      bytes);
    cudaMalloc(&d_grad_x, bytes);
    cudaMalloc(&d_grad_y, bytes);

    for (int c = 0; c < data.C; c++) {
        float* channel = data.data_ptr() + c * data.n_pixels();
        cudaMemcpy(d_u, channel, bytes, cudaMemcpyDeviceToDevice);
        
        for (int iter = 0; iter < iterations; iter++) {
            tvd_compute_gradients<<<grid, block>>>(
                d_u, d_grad_x, d_grad_y, H, W
            );
            tvd_update<<<grid, block>>>(
                channel, d_u, d_grad_x, d_grad_y, lambda, dt, H, W
            );
        }
        cudaMemcpy(channel, d_u, bytes, cudaMemcpyDeviceToDevice);
    }

    cudaFree(d_u);
    cudaFree(d_grad_x);
    cudaFree(d_grad_y);
}

