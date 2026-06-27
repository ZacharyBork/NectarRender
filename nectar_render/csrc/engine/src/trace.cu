#include "engine/include/engine/trace.h"

#include "engine/include/engine/light.h"

// ============================================================================
// RAY TRACING FUNCTION
// ============================================================================

__device__ bool trace_ray(
    Scene  scene,
    Ray&   ray,
    Color& color,
    Color& atten,
    Generator& gen
) {
    HitRecord rec;
    bool hit = scene.hit(ray, Interval(EPS, FMAX), rec);
    
    if (!hit) {
        color += atten * scene.skylight.sample(ray);
        return false;
    }

    color += atten * rec.material->emitted(rec.uv, rec.position);
    return hit & rec.material->scatter(rec, ray, atten, gen);
}

__global__ void trace_kernel(
    Scene        scene,
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

    for (int bounce = 0; bounce < max_depth; bounce++)
        if (!trace_ray(scene, ray, color, atten, gen)) break;

    data.set_color(p_idx.x, p_idx.y, p_idx.z, color);
}

void trace(
    Scene        scene,
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
