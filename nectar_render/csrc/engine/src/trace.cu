#include "engine/include/engine/trace.h"

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

/* HITTABLES */

template Sphere* device_build<Sphere>(
    Vector3, Vector3, float, Material*
);

/* MATERIALS */

template Lambertian* device_build<Lambertian>(Color);
template Metal*      device_build<Metal>(Color, float);
template Dielectric* device_build<Dielectric>(float);

// ============================================================================
// RAY COLORING
// ============================================================================

__device__ Color sample_skylight(const Ray& ray) {
    Vector3 unit_direction = normalize(ray.direction());
    float a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) 
         * Color(1.0, 1.0, 1.0) 
         + a * Color(0.5, 0.7, 1.0);
}

// ============================================================================
// RAY TRACING FUNCTION
// ============================================================================

__global__ void trace_kernel(
    const Scene  scene,
    Camera       camera,
    DataObject   data, 
    unsigned int max_depth,
    unsigned int seed,
    unsigned int frame
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= data.W || p_idx.y >= data.H) return;
    
    unsigned int pixel_idx = p_idx.y * data.W + p_idx.x;
    Generator gen(seed, pixel_idx + frame * (data.W * data.H));

    Ray ray = camera.get_ray(p_idx.x, p_idx.y, gen);
    
    Color color = Color::black();
    Color atten = Color::white();

    for (int bounce = 0; bounce < max_depth; bounce++) {
        HitRecord rec;
        bool hit = scene.hit(ray, Interval(EPS, FMAX), rec);
        
        if (!hit) {
            color += atten * sample_skylight(ray);
            break;
        }

        rec.material->scatter(rec, ray, atten, gen);
    }

    data.set_color(p_idx.x, p_idx.y, p_idx.z, color);
}

void trace(
    const Scene  scene,
    Camera       camera,
    DataObject   data, 
    unsigned int max_depth,
    unsigned int seed, 
    unsigned int frame
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    trace_kernel<<<grid, block>>>(scene, camera, data, max_depth, seed, frame);
}
