#include "engine/include/engine/trace.h"
#include "engine/include/engine/pdf.h"

// ============================================================================
// TRACE SHADOW RAYS
// ============================================================================

__device__ Color trace_shadow_rays(
    SceneGraph* scene,
    HitRecord&  rec,
    uint32_t    n_shadow_rays,
    Generator&  gen
) {
    Color shadow_atten = Color::white();

    for (int i = 0; i < n_shadow_rays; i++) {
        Hittable* light = scene->lights[gen.random_int(0, scene->n_lights)];

        Vector3 to_light  = light->random(rec.p, gen);
        float   dist      = to_light.length();
        Vector3 light_dir = to_light / dist;

        HitRecord tmp_rec;
        Ray r_shadow(rec.p, light_dir);
        bool occluded = scene->hit(r_shadow, Interval(EPS, dist-EPS), tmp_rec);

        if (occluded) {
            shadow_atten -= 1.0f / (float)n_shadow_rays;
        }
    }

    return shadow_atten;
}

// ============================================================================
// TRACE SINGLE RAY
// ============================================================================

__device__ bool sample_cosine_brdf(
    SceneGraph*    scene,
    Ray&           ray,
    Ray&           r_in,
    Color&         atten,
    HitRecord&     rec,
    ScatterRecord& srec,
    Generator&     gen
) {
    Vector3 direction;
    float   pdf_value;

    if (scene->n_lights > 0) {
        Hittable** lights = reinterpret_cast<Hittable**>(scene->lights);
        MixturePDF pdf(srec.pdf, PDF::hittable(lights, rec.p));
        direction = pdf.generate(gen);
        pdf_value = pdf.value(direction);
    } else {
        direction = srec.pdf.generate(gen);
        pdf_value = srec.pdf.value(direction);
    }

    if (pdf_value <= 0.0f) return false;
    
    ray = Ray(rec.p, direction, r_in.time());
    Color brdf = rec.mat->evaluate(
        rec, normalize(-r_in.direction()), direction
    );
    atten *= brdf / pdf_value;

    return true;
}

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
        aovs->beauty += atten * scene->skylight->sample(ray);
        return false;
    }

    ScatterRecord srec;
    Ray r_in = ray.clone();

    aovs->beauty += atten * rec.mat->emitted(ray, rec);
    if (!rec.mat->scatter(rec, ray, srec, gen)) {
        return false;
    }

    if (srec.skip_pdf) {
        atten *= srec.atten;
        ray = srec.skip_pdf_ray;
        return true;
    }

    return sample_cosine_brdf(scene, ray, r_in, atten, rec, srec, gen);
}

// ============================================================================
// LIGHT TRANSPORT LOOP
// ============================================================================

__global__ void trace_full_kernel(
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs,
    uint32_t      sample_idx,
    uint32_t      ray_depth,
    uint32_t      seed
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= cam->W || p_idx.y >= cam->H) return;
    uint32_t pixel_idx = p_idx.y * cam->W + p_idx.x;

    Color atten = Color::white();
    Generator gen(seed, pixel_idx + sample_idx * (cam->W * cam->H));
    Ray ray = cam->get_ray(
        p_idx.x, p_idx.y, pixel_idx, sample_idx, seed, gen
    );

    for (int bounce = 0; bounce < ray_depth; bounce++)
        if (!trace_ray(scene, aovs, ray, atten, gen)) break;
}

void trace_full(
    Camera&     cam,
    SceneGraph* scene,
    AOVs*       aovs,
    uint32_t    sample_idx,
    uint32_t    ray_depth,
    uint32_t    seed
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid(
        (cam.width()  + BS2D - 1) / BS2D, 
        (cam.height() + BS2D - 1) / BS2D, 
        1
    );
    trace_full_kernel<<<grid, block>>>(
        cam.device_camera(), scene, aovs, sample_idx, ray_depth, seed
    );

}

// ============================================================================
// VIEWPORT RENDERING
// ============================================================================

__global__ void trace_viewport_kernel(
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs,
    uint32_t      seed
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= cam->W || p_idx.y >= cam->H) return;
    uint32_t pixel_idx = p_idx.y * cam->W + p_idx.x;

    Generator gen(seed, pixel_idx * (cam->W * cam->H));
    Ray ray = cam->get_ray(
        p_idx.x, p_idx.y, pixel_idx, 0u, seed, gen
    );

    HitRecord rec;
    bool hit = scene->hit(ray, Interval(EPS, FMAX), rec);

    if (!hit) {
        aovs->beauty += scene->skylight->sample(ray);
        return;
    }

    Vector3 light_direction(-1.0f, -1.0f, -1.0f);
    Color viewport_color(0.0f, 0.0f, 0.0f);

    viewport_color += rec.mat->viewport_color(rec);
    viewport_color *= dot(light_direction, -rec.n) * 0.5f + 0.5f;

    aovs->beauty += viewport_color;
}

void trace_viewport(
    Camera&     cam,
    SceneGraph* scene,
    AOVs*       aovs,
    uint32_t    seed
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid(
        (cam.width()  + BS2D - 1) / BS2D, 
        (cam.height() + BS2D - 1) / BS2D, 
        1
    );
    trace_viewport_kernel<<<grid, block>>>(
        cam.device_camera(), scene, aovs, seed
    );
}

// ============================================================================
// HIT TEST RAY
// ============================================================================

__global__ void hit_test_ray_kernel(
    float u, 
    float v, 
    SceneGraph* scene, 
    DeviceCamera* cam,
    HitRecord* rec_out
) {
    HitRecord rec;
    Ray ray = cam->screen_space_ray(u, v);
    bool hit = scene->hit(ray, Interval(EPS, FMAX), rec);
    *rec_out = rec;
}

void hit_test_ray(
    float u, 
    float v, 
    SceneGraph* scene, 
    DeviceCamera* cam,
    HitRecord* rec
) {
    hit_test_ray_kernel<<<1, 1>>>(u, v, scene, cam, rec);
}





