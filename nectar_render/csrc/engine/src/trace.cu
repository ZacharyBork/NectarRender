#include "engine/include/engine/trace.h"

// ============================================================================
// RAY TRACING FUNCTION
// ============================================================================

__device__ bool trace_ray(
    SceneGraph scene,
    Ray&       ray,
    Color&     color,
    Color&     atten,
    Generator& gen
) {
    HitRecord rec;
    bool hit = scene.hit(ray, Interval(EPS, FMAX), rec);

    if (!hit) {
        color += atten * scene.skylight.sample(ray);
        return false;
    }

    color += atten * rec.mat->emitted(rec.uv, rec.p);
    return hit & rec.mat->scatter(rec, ray, atten, gen);
}

__global__ void trace_kernel(
    Camera       camera,
    SceneGraph   scene,
    AOVs         aovs, 
    unsigned int max_depth,
    unsigned int seed,
    unsigned int frame
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= aovs.W || p_idx.y >= aovs.H) return;
    unsigned int pixel_idx = p_idx.y * aovs.W + p_idx.x;

    Generator gen(seed, pixel_idx + frame * (aovs.W * aovs.H));
    Ray ray = camera.get_ray(p_idx.x, p_idx.y, gen);

    Color color = Color::black();
    Color atten = Color::white();

    for (int bounce = 0; bounce < max_depth; bounce++)
        if (!trace_ray(scene, ray, color, atten, gen)) break;

    aovs.beauty.set_color(color);
}

void trace(
    Camera        camera,
    Scene&        scene,
    RenderLayers& layers, 
    unsigned int  max_depth,
    unsigned int  seed, 
    unsigned int  frame
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((layers.W + BS2D - 1) / BS2D, (layers.H + BS2D - 1) / BS2D, 1);
    trace_kernel<<<grid, block>>>(
        camera, scene.graph(), layers.aovs(), max_depth, seed, frame
    );
}
