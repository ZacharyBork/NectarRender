#include "scene/include/interface.h"

__global__ void mask_selected_kernel(
    uint8_t*      base_mask,
    size_t        H, 
    size_t        W,
    DeviceCamera* cam,
    SceneGraph*   scene,
    size_t        selected_object_id
) {

    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= W || p_idx.y >= H) return;
    uint32_t pixel_idx = p_idx.y * W + p_idx.x;

    float su = (p_idx.x + 0.5f) / (float)W;
    float sv = (p_idx.y + 0.5f) / (float)H;
    Ray ray = cam->screen_space_ray(su, sv);

    HitRecord rec;
    bool hit = scene->hit(ray, Interval(EPS, FMAX), rec);
    base_mask[pixel_idx] = (
        hit && rec.object_id == selected_object_id
    ) ? 255u : 0u;
}

__global__ void build_outline_kernel(
    uint8_t* base_mask,
    uint8_t* out_mask,
    size_t H, 
    size_t W,
    int    outline_radius
) {
    ProcessIndex p_idx = get_process_index();
    if (p_idx.x >= W || p_idx.y >= H) return;
    uint32_t idx = p_idx.y * W + p_idx.x;
    out_mask[idx] = 0u;

    if (base_mask[idx] != 0) return;

    for (int dy = -outline_radius; dy <= outline_radius; dy++) {
        for (int dx = -outline_radius; dx <= outline_radius; dx++) {
            int nx = p_idx.x + dx, ny = p_idx.y + dy;
            
            if (nx < 0 || ny < 0 || nx >= W || ny >= H) 
                continue;
            
            if (base_mask[ny * W + nx] != 0) {
                out_mask[idx] = 255u;
                return;
            }
        }
    }
}

uint8_t* selection_mask(
    size_t        H, 
    size_t        W,
    DeviceCamera* cam,
    SceneGraph*   scene,
    size_t        selected_object_id,
    int           outline_radius
) {
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((W + BS2D - 1) / BS2D, (H + BS2D - 1) / BS2D, 1);

    uint8_t* base_mask;
    CUDAMemory::allocate<uint8_t>(base_mask, H * W);
    mask_selected_kernel<<<grid, block>>>(
        base_mask, H, W, cam, scene, selected_object_id
    );

    uint8_t* out_mask;
    CUDAMemory::allocate<uint8_t>(out_mask, H * W);
    build_outline_kernel<<<grid, block>>>(
        base_mask, out_mask, H, W, outline_radius
    );

    CUDAMemory::free(base_mask);
    return out_mask;
}


