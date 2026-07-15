#include "hittable/include/hittable/hittable.h"

// ============================================================================
// TRANSFORM UTILITIES
// ============================================================================

__global__ void get_xform_kernel(
    Hittable*  hittable,
    Transform& xform
) {
    xform = hittable->xform;
}

void run_get_xform(
    Hittable*  hittable,
    Transform& xform
) {
    get_xform_kernel<<<1, 1>>>(hittable, xform);
}

__global__ void update_xform_kernel(
    Hittable*  hittable,
    Transform& xform
) {
    hittable->xform = xform;
}

void run_update_xform(
    Hittable*  hittable,
    Transform& xform
) {
    update_xform_kernel<<<1, 1>>>(hittable, xform);
}

// ============================================================================
// MATERIAL UTILITIES
// ============================================================================

__global__ void update_material_kernel(
    Hittable* hittable,
    Material* mat
) {
    hittable->material = mat;
}

void run_update_material(
    Hittable* hittable,
    Material* mat
) {
    update_material_kernel<<<1, 1>>>(hittable, mat);
}

