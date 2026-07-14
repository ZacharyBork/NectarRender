#include "interface/include/object_interface.h"

__global__ void generate_mask_kernel(
    DataView      data,
    DeviceCamera* cam,
    SceneGraph*   scene,
    Hittable*     selected_object,
    float*        mask_out
) {

    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= data.W || p_idx.y >= data.H) return;
    uint32_t pixel_idx = p_idx.y * data.W + p_idx.x;

    if (!selected_object) { mask_out[pixel_idx] = 0.0f; return; }

    float su = (p_idx.x + 0.5f) / (float)data.W;
    float sv = (p_idx.y + 0.5f) / (float)data.H;
    Ray ray = cam->screen_space_ray(su, sv);

    HitRecord rec;
    bool hit = scene->hit(ray, Interval(EPS, FMAX), rec);
    mask_out[pixel_idx] = (
        hit && rec.hit_object == selected_object
    ) ? 1.0f : 0.0f;
}

__global__ void composite_mask_kernel(
    DataView data,
    float*   mask,
    int      outline_radius,
    uint8_t  r, 
    uint8_t  g,
    uint8_t  b
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= data.W || p_idx.y >= data.H) return;
    uint32_t idx = p_idx.y * data.W + p_idx.x;

    if (mask[idx] != 0) return;

    for (int dy = -outline_radius; dy <= outline_radius; dy++) {
        for (int dx = -outline_radius; dx <= outline_radius; dx++) {
            int nx = (int)p_idx.x + dx, ny = (int)p_idx.y + dy;
            
            if (nx < 0 || ny < 0 || nx >= (int)data.W || ny >= (int)data.H) 
                continue;
            
                if (mask[ny * data.W + nx] != 0) {
                    data.set_color(r, g, b);
                    return;
                }
        }
    }
}

void selection_mask(
    DataView      data,
    DeviceCamera* cam,
    SceneGraph*   scene,
    Hittable*     selected_object,
    int      outline_radius,
    uint8_t  r, 
    uint8_t  g,
    uint8_t  b
) {
    float* d_mask_ptr;
    cudaMalloc(&d_mask_ptr, data.H * data.W * sizeof(float));
   
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    
    generate_mask_kernel<<<grid, block>>>(
        data, cam, scene, selected_object, d_mask_ptr
    );
    composite_mask_kernel<<<grid, block>>>(
        data, d_mask_ptr, outline_radius, 255u, 120u, 45u
    );
}


