#include "core/include/core.h"
#include "engine/include/engine/ray.h"

__device__ float grid_mask(float u, float v, float spacing, float width) {
    float du = fabsf(fmodf(u + spacing * 0.5f, spacing) - spacing * 0.5f);
    float dv = fabsf(fmodf(v + spacing * 0.5f, spacing) - spacing * 0.5f);
    float d  = fminf(du, dv);
    return 1.0f - CG::smoothstep(0.0f, width, d);
}

__device__ bool axis_grid(
    const Ray& ray, 
    float  max_t, 
    int    axis, 
    Color  major_color,
    Color& grid_color, 
    float& grid_alpha,
    float  line_width = 5e-4f
) {
    float dir_axis = ray.direction()[axis];
    if (fabsf(dir_axis) < 1e-6f) return false;

    float t = -ray.origin()[axis] / dir_axis;
    if (t <= 0.0f || t >= max_t) return false;

    Vector3 p = ray.at(t);
    p = Vector3(fabsf(p.x()), fabsf(p.y()), fabsf(p.z()));
    int u_axis = (axis + 1) % 3, v_axis = (axis + 2) % 3;
    float u = p[u_axis], v = p[v_axis];

    float pixel_size = line_width * t; 

    float level  = fmaxf(log10f(fmaxf(pixel_size, 1e-5f)) + 1.0f, 0.0f);
    float level0 = floorf(level);
    float blend  = level - (float)level0;

    float spacing0 = powf(10.0f, (float)level0);
    float spacing1 = spacing0 * 10.0f;
    float width = pixel_size * 1.5f;

    float g0 = grid_mask(u, v, spacing0, width);
    float g1 = grid_mask(u, v, spacing1, width);

    float mask = fmaxf(g0 * (1.0f - blend), g1);
    if (mask < 0.01f) return false;

    float fade = fmaxf(0.0f, 1.0f - t / 100.0f);
    grid_color = CG::lerp(
        Color(0.35f), major_color, CG::smoothstep(0.0f, 1.0f, g1)
    );
    grid_alpha = mask * fade;
    return true;
}

__device__ bool triplanar_grid(
    const Ray& ray, 
    float max_t, 
    Color& grid_color, 
    float& grid_alpha
) {
    bool hit_grid = false;
    hit_grid = hit_grid || axis_grid(
        ray, max_t, 0, Color(1.0f, 0.3f, 0.3f), grid_color, grid_alpha
    );
    hit_grid = hit_grid || axis_grid(
        ray, max_t, 1, Color(0.3f, 1.0f, 0.3f), grid_color, grid_alpha
    );
    hit_grid = hit_grid || axis_grid(
        ray, max_t, 2, Color(0.3f, 0.3f, 1.0f), grid_color, grid_alpha
    );
    return hit_grid;
}

