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
    cudaMalloc(&d_ptr, sizeof(T));
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

// TEXTURES ===================================================================

template ConstantTexture* device_build<ConstantTexture>(Color);
template CheckerTexture*  device_build<CheckerTexture>(Color, Color, float);
template NoiseTexture*    device_build<NoiseTexture>(Perlin*, float, int);
template ImageTexture*    device_build<ImageTexture>(
    uint8_t*, size_t, size_t, size_t
);

// MATERIALS ================================================================== 

template Lambertian* device_build<Lambertian>(Texture*);
template Dielectric* device_build<Dielectric>(float);
template Emissive*   device_build<Emissive>(Texture*, float);
template Isotropic*  device_build<Isotropic>(Texture*);

template PBR* device_build<PBR>(
    Texture*, Texture*, Texture*, Texture*, Texture*
);

template Material* device_build<Material>(MaterialType, void*);

