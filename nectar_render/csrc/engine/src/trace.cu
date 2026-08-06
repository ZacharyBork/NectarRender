#include "engine/include/engine/trace.h"
#include "engine/include/engine/pdf.h"

#include "brdf.cu"
#include "axis_grid.cu"

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
    bool          show_axis_grid,
    uint32_t      seed
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= cam->W || p_idx.y >= cam->H) return;

    HitRecord rec;
    Ray ray = cam->get_ray_center(p_idx.x, p_idx.y);
    bool hit = scene->hit(ray, Interval(EPS, FMAX), rec);

    if (!hit) {
        Color grid_color; float grid_alpha;
        if (show_axis_grid
        && triplanar_grid(ray, FMAX, grid_color, grid_alpha)) {
            Color sky = scene->skylight->sample(ray);
            aovs->beauty += CG::lerp(sky, grid_color, grid_alpha);
            return;
        } else {
            aovs->beauty += scene->skylight->sample(ray);
            return;
        }
    }

    Vector3 light_vector = cam->p.position - rec.p;
    float light_dist = light_vector.length();

    Vector3 headlight_dir = normalize(light_vector);
    float headlight = dot(headlight_dir, rec.n) * 0.5f + 0.5f;
    headlight *= 1.0f / (light_dist * light_dist);
    headlight = fminf(0.75f, headlight);

    Vector3 directional_light_dir = Vector3(1.0f, 1.0f, 1.0f);
    float directional = (dot(directional_light_dir, rec.n) + 1.0f) * 0.5f;
    directional = fmaxf(0.1f, directional);

    Color col = rec.mat->viewport_color(rec);
    aovs->beauty += col * fmaxf(headlight, directional);
}

void trace_viewport(
    Camera&     cam,
    SceneGraph* scene,
    AOVs*       aovs,
    bool        show_axis_grid,
    uint32_t    seed
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid(
        (cam.width()  + BS2D - 1) / BS2D, 
        (cam.height() + BS2D - 1) / BS2D, 
        1
    );
    trace_viewport_kernel<<<grid, block>>>(
        cam.device_camera(), scene, aovs, show_axis_grid, seed
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





