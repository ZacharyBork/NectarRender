#include "engine/include/engine/trace.h"

// ============================================================================
// RAY TRACING FUNCTION
// ============================================================================

__device__ bool trace_ray(
    SceneGraph* scene,
    AOVs*       aovs,
    Ray&        ray,
    Color&      atten,
    Generator&  gen
) {
    HitRecord rec;
    bool hit = scene->hit(ray, Interval(EPS, FMAX), rec);

    if (!hit) {
        aovs->beauty += atten * scene->skylight.sample(ray);
        return false;
    }

    aovs->beauty += atten * rec.mat->emitted(rec.uv, rec.p);
    return hit & rec.mat->scatter(rec, ray, atten, gen);
}

__global__ void trace_kernel(TraceConfig cfg) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= cfg.W || p_idx.y >= cfg.H) return;
    uint32_t pixel_idx = p_idx.y * cfg.W + p_idx.x;

    Color atten = Color::white();
    Generator gen(cfg.seed, pixel_idx + cfg.frame * (cfg.W * cfg.H));
    Ray ray = cfg.camera->get_ray(p_idx.x, p_idx.y, cfg.s_x, cfg.s_y, gen);

    for (int bounce = 0; bounce < cfg.max_depth; bounce++)
        if (!trace_ray(cfg.scene, cfg.aovs, ray, atten, gen)) break;
}

void trace(TraceConfig cfg) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((cfg.W + BS2D - 1) / BS2D, (cfg.H + BS2D - 1) / BS2D, 1);
    trace_kernel<<<grid, block>>>(cfg);
}

