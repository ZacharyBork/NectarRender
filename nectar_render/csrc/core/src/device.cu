#include "core/include/core/device.h"

#include "engine/include/engine/scene.h"
#include "hittable/include/hittable.h"
#include "engine/include/engine/light.h"

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

template Quad* device_build<Quad>(Transform, Transform, size_t);
template Quad* device_build<Quad>(Vector3, Vector3, Vector3, size_t);

template Sphere* device_build<Sphere>(Transform, Transform, float, size_t);
template Cube*   device_build<Cube>(
    Transform, Transform, Hittable**, size_t
);

template ConstantMedium* device_build<ConstantMedium>(
    Transform, Transform, Hittable*, float, size_t
);

template Mesh* device_build<Mesh>(
    Transform, Transform, size_t, MeshVertex*, BVHNode*, TriangleRef*
);

// LIGHTS =====================================================================

template ObjectLight* device_build<ObjectLight>(
    Transform, Transform, Hittable*, size_t
);

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

