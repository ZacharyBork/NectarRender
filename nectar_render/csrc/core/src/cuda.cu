#include "core/include/core/cuda.h"

#include "hittable/include/hittable.h"
#include "engine/include/engine/light.h"
#include "engine/include/engine/scene.h"

// ############################################################################
// DEVICE-SIDE OBJECT CONSTRUCTION
// ############################################################################

template<typename T, typename... Args>
__global__ void device_build_kernel(T* d_obj, Args... args) {
    new (d_obj) T(args...);
}

template<typename T, typename... Args>
T* device_build(Args... args) {
    T* d_ptr;
    CUDAMemory::allocate<T>(d_ptr);
    device_build_kernel<<<1, 1>>>(d_ptr, args...);
    cudaDeviceSynchronize();
    return d_ptr;
}

// HITTABLES ==================================================================

template Quad*   device_build<Quad>();
// template Quad*   device_build<Quad>(Vector3, Vector3, Vector3, size_t, size_t);
template Sphere* device_build<Sphere>(float);
template Cube*   device_build<Cube>(Quad**);
template ConstantMedium* device_build<ConstantMedium>(float);
template Mesh* device_build<Mesh>(MeshVertex*, BVHNode*, TriangleRef*);

template Hittable* device_build<Hittable>(
    HittableType, Transform, Transform, size_t, size_t, Hittable*, void*
);

// LIGHTS =====================================================================

template ObjectLight* device_build<ObjectLight>();

// NOISE ======================================================================

template Perlin* device_build<Perlin>(
    Vector3* rv, int* perm_x, int* perm_y, int* perm_z
);

// MATERIALS ================================================================== 

template Lambertian* device_build<Lambertian>(TextureView);
template Dielectric* device_build<Dielectric>(float);
template Emissive*   device_build<Emissive>(TextureView, float);
template Isotropic*  device_build<Isotropic>(TextureView);

template PBR* device_build<PBR>(
    TextureView, TextureView, TextureView, TextureView, TextureView
);

template Material* device_build<Material>(MaterialType, void*);

// ############################################################################
// MEMORY MANAGEMENT
// ############################################################################

template<typename T>
__global__ void fill_memory_kernel(T* d_ptr, T value, size_t n_elements) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_elements) return;
    d_ptr[idx] = value;
}

template<typename T>
void run_fill_memory(T* d_ptr, T value, size_t n_elements) {
    fill_memory_kernel<<<1, 1>>>(d_ptr, value, n_elements);
}

template void run_fill_memory<float>(float*, float, size_t);
template void run_fill_memory<int>(int*, int, size_t);
template void run_fill_memory<size_t>(size_t*, size_t, size_t);
template void run_fill_memory<uint8_t>(uint8_t*, uint8_t, size_t);
template void run_fill_memory<uint32_t>(uint32_t*, uint32_t, size_t);

template void run_fill_memory<Vector2>(Vector2*, Vector2, size_t);
template void run_fill_memory<Vector3>(Vector3*, Vector3, size_t);
template void run_fill_memory<Color>(Color*, Color, size_t);


