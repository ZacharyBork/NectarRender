#include "core/include/core/vector.h"
#include "core/include/core/random.h"

#include "engine/include/engine/trace.h"
#include "engine/include/engine/coordinates.h"
#include "engine/include/engine/ray.h"

__device__ Color ray_color(const Ray& ray) {
    Vector3 unit_direction = unit_vector(ray.direction());
    auto a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) 
         * Color(1.0, 1.0, 1.0) 
         + a * Color(0.5, 0.7, 1.0);
}

__global__ void trace_kernel(
    DataObject   data, 
    unsigned int seed,
    unsigned int frame
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int z = blockIdx.z;
    if (x >= data.W || y >= data.H) return;
    
    unsigned int pixel_idx = y * data.W + x;
    Generator rng(seed, pixel_idx + frame * (data.W * data.H));

    float u1 = rng.uniform();
    float u2 = rng.uniform();
    float u3 = rng.uniform();

    UVSample uv(x, y, data.H, data.W);
    // Color col(uv.u, uv.v, 0.0);
    Color col(u1, u2, u3);
    data.set_color(x, y, z, col);
}

void trace(
    DataObject   data, 
    unsigned int seed, 
    unsigned int frame
) {
    int BS2D = 16;
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    trace_kernel<<<grid, block>>>(data, seed, frame);
}


