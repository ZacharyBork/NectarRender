#include "engine/include/engine/trace.h"
#include "engine/include/engine/pdf.h"

// ============================================================================
// PROBABILITY DENSITY FUNCTION
// ============================================================================

__device__ MixturePDF build_pdf(
    SceneGraph* scene,
    HitRecord&  rec,
    Generator&  gen
) {
    HittablePDF<Light> light_pdf(scene->lights, rec.p, gen);
    CosinePDF cosine_pdf(rec.n, gen);
    MixturePDF pdf(&light_pdf, &cosine_pdf, gen);
    return pdf;
}

// ============================================================================
// TRACE SINGLE RAY
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

    Ray r_in = ray.clone();
    Color sample_color = Color::white();
    
    aovs->beauty += atten * rec.mat->emitted(ray, rec);
    if (!rec.mat->scatter(rec, ray, sample_color, gen)) {
        return false;
    }

    MixturePDF pdf = build_pdf(scene, rec, gen);
    if (pdf.value <= 0.0f) return false;
    
    ray = Ray(rec.p, pdf.direction, r_in.time());
    float scatter_pdf = rec.mat->scattering_pdf(rec, r_in, ray);
    atten *= (scatter_pdf * sample_color) / pdf.value;

    return true;
}

// ============================================================================
// LIGHT TRANSPORT LOOP
// ============================================================================

__global__ void trace_kernel(
    TraceConfig   cfg, 
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= cfg.W || p_idx.y >= cfg.H) return;
    uint32_t pixel_idx = p_idx.y * cfg.W + p_idx.x;

    Color atten = Color::white();
    Generator gen(cfg.seed, pixel_idx + cfg.n_samples * (cfg.W * cfg.H));
    Ray ray = cam->get_ray(p_idx.x, p_idx.y, cfg.s_x, cfg.s_y, gen);

    for (int bounce = 0; bounce < cfg.max_depth; bounce++)
        if (!trace_ray(scene, aovs, ray, atten, gen)) break;
}

void trace(
    TraceConfig   cfg, 
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((cfg.W + BS2D - 1) / BS2D, (cfg.H + BS2D - 1) / BS2D, 1);
    trace_kernel<<<grid, block>>>(cfg, cam, scene, aovs);
}

