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
        Light* light = scene->lights[gen.random_int(0, scene->n_lights)];

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
// OUTLIER REJECTION
// ============================================================================

__device__ inline float luminance(const Color& c) {
    return 0.2126f * c.r() + 0.7152f * c.g() + 0.0722f * c.b();
}

__global__ void reject_outliers_kernel(
    DataView beauty,
    float*   dst,
    float    threshold
) {
    size_t H = beauty.H, W = beauty.W;
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= W || p_idx.y >= H) return;

    Color center = beauty.get_color();

    float neighbor_lum[8];
    int n = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = p_idx.x + dx, ny = p_idx.y + dy;
            if (nx < 0 || ny < 0 || nx >= (int)W || ny >= (int)H) continue;
            size_t plane = beauty.H * beauty.W;
            uint32_t nidx = ny * W + nx;
            Color neighbor(
                beauty.ptr[0 * plane + nidx],
                beauty.ptr[1 * plane + nidx],
                beauty.ptr[2 * plane + nidx]
            );
            neighbor_lum[n++] = luminance(neighbor);
        }
    }

    for (int i = 1; i < n; i++) {
        float key = neighbor_lum[i];
        int j = i - 1;
        while (j >= 0 && neighbor_lum[j] > key) {
            neighbor_lum[j + 1] = neighbor_lum[j];
            j--;
        }
        neighbor_lum[j + 1] = key;
    }
    float median = (n > 0) ? neighbor_lum[n / 2] : 0.0f;

    Color result;
    float center_lum = luminance(center);
    if (median > 1e-6f && center_lum > threshold * median) {
        result = center * (threshold * median / center_lum);
    } else {
        result = center;
    }

    ColorIndex c_idx(beauty.C, beauty.H, beauty.W);
    dst[c_idx.r] = result.r();
    dst[c_idx.g] = result.g();
    dst[c_idx.b] = result.b();
}

void reject_outliers(RenderLayers& layers, float threshold) {
    size_t H = layers.beauty.H, W = layers.beauty.W;
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((W + BS2D - 1) / BS2D, (H + BS2D - 1) / BS2D, 1);

    float* result;
    cudaMalloc(&result, layers.beauty.n_bytes());
    reject_outliers_kernel<<<grid, block>>>(
        layers.beauty.view(), result, threshold
    );
    layers.beauty.overwrite(result);
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
        aovs->beauty += atten * scene->skylight.sample(ray);
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

__global__ void trace_kernel(
    TraceConfig   cfg, 
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs,
    uint32_t      sample_idx
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= cfg.W || p_idx.y >= cfg.H) return;
    uint32_t pixel_idx = p_idx.y * cfg.W + p_idx.x;

    Color atten = Color::white();
    Generator gen(cfg.seed, pixel_idx + sample_idx * (cfg.W * cfg.H));
    Ray ray = cam->get_ray(
        p_idx.x, p_idx.y, pixel_idx, sample_idx, cfg.seed, gen
    );

    for (int bounce = 0; bounce < cfg.max_depth; bounce++)
        if (!trace_ray(scene, aovs, ray, atten, gen)) break;
}

void trace(
    TraceConfig   cfg, 
    DeviceCamera* cam,
    SceneGraph*   scene,
    AOVs*         aovs,
    uint32_t      sample_idx
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((cfg.W + BS2D - 1) / BS2D, (cfg.H + BS2D - 1) / BS2D, 1);
    trace_kernel<<<grid, block>>>(cfg, cam, scene, aovs, sample_idx);
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





