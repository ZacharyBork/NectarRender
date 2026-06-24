#include "engine/include/engine/trace.h"
#include "engine/include/engine/ray.h"

#include "core/include/core/constants.h"
#include "core/include/core/vector.h"
#include "core/include/core/interval.h"
#include "core/include/core/random.h"

// ============================================================================
// DEVICE
// ============================================================================

template<typename T, typename... Args>
__global__ void device_build_kernel(T* d_obj, Args... args) {
    new (d_obj) T(args...);
}

template<typename T, typename... Args>
T* device_build(Args... args) {
    T* d_ptr;
    cudaMalloc(&d_ptr, sizeof(T));
    device_build_kernel<<<1, 1>>>(d_ptr, args...);
    cudaDeviceSynchronize();
    return d_ptr;
}

template Sphere* device_build<Sphere, Point3, float>(Point3, float);

// ============================================================================
// RAY COLORING
// ============================================================================

__device__ Color ray_color(const Ray& ray, const HittablesList world) {
    HitRecord rec;

    if (world.hit(ray, Interval(EPS, FMAX), rec)) {
        return 0.5 * (rec.normal + Color(1.0f, 1.0f, 1.0f));
    }

    Vector3 unit_direction = unit_vector(ray.direction());
    float a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) 
         * Color(1.0, 1.0, 1.0) 
         + a * Color(0.5, 0.7, 1.0);
}

// ============================================================================
// RAY TRACING FUNCTION
// ============================================================================

__global__ void trace_kernel(
    const HittablesList world,
    Camera       camera,
    DataObject   data, 
    unsigned int seed,
    unsigned int frame
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= data.W || p_idx.y >= data.H) return;
    
    unsigned int pixel_idx = p_idx.y * data.W + p_idx.x;
    Generator gen(seed, pixel_idx + frame * (data.W * data.H));

    Ray ray = camera.spawn_rays(p_idx.x, p_idx.y, gen);
    Color col = ray_color(ray, world);

    data.set_color(p_idx.x, p_idx.y, p_idx.z, col);
}

void trace(
    const HittablesList world,
    Camera       camera,
    DataObject   data, 
    unsigned int seed, 
    unsigned int frame
) {
    int BS2D = 16;
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    trace_kernel<<<grid, block>>>(world, camera, data, seed, frame);
}


