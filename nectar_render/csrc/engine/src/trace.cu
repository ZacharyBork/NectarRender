#include "engine/include/engine/trace.h"
#include "engine/include/engine/coordinates.h"
#include "engine/include/engine/ray.h"

#include "core/include/core/constants.h"
#include "core/include/core/vector.h"
#include "core/include/core/random.h"

// ============================================================================
// DEVICE
// ============================================================================

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

template Sphere* device_build<Sphere, Point3, float>(Point3, float);

// ============================================================================
// RAY COLORING
// ============================================================================

__device__ Color ray_color(const Ray& ray, const HittablesList world) {
    HitRecord rec;

    if (world.hit(ray, 0, FMAX, rec)) {
        return 0.5 * (rec.normal + Color(1.0f, 1.0f, 1.0f));
    }

    Vector3 unit_direction = unit_vector(ray.direction());
    float a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) 
         * Color(1.0, 1.0, 1.0) 
         + a * Color(0.5, 0.7, 1.0);
}

// ============================================================================
// RAY TRACING FUNCTION
// ============================================================================

__global__ void trace_kernel(
    const HittablesList world,
    DataObject   data, 
    unsigned int seed,
    unsigned int frame
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int z = blockIdx.z;
    if (x >= data.W || y >= data.H) return;
    
    unsigned int pixel_idx = y * data.W + x;
    Generator gen(seed, pixel_idx + frame * (data.W * data.H));
    UVSample uv(x, y, data.H, data.W);

    // ------------------------------------------------------------------------

    // Camera

    float focal_length = 1.0f;
    float viewport_height = 2.0f;
    float viewport_width = viewport_height * ((float)data.W / (float)data.H);
    Point3 camera_center = Point3(0.0f, 0.0f, 0.0f);

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    Vector3 viewport_u = Vector3(viewport_width, 0.0f, 0.0f);
    Vector3 viewport_v = Vector3(0.0f, -viewport_height, 0.0f);

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    Vector3 pixel_delta_u = viewport_u / data.W;
    Vector3 pixel_delta_v = viewport_v / data.H;

    // Calculate the location of the upper left pixel.
    Vector3 viewport_upper_left = camera_center
                                - Vector3(0.0f, 0.0f, focal_length)
                                - viewport_u / 2.0f 
                                - viewport_v / 2.0f;
    Vector3 pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

    Vector3 pixel_center = pixel00_loc + (x * pixel_delta_u) + (y * pixel_delta_v);
    Vector3 ray_direction = pixel_center - camera_center;
    
    Ray ray(camera_center, ray_direction);
    Color col = ray_color(ray, world);

    // ------------------------------------------------------------------------
    
    data.set_color(x, y, z, col);
}

void trace(
    const HittablesList world,
    DataObject   data, 
    unsigned int seed, 
    unsigned int frame
) {
    int BS2D = 16;
    dim3 block(BS2D, BS2D, 1);
    dim3 grid((data.W + BS2D - 1) / BS2D, (data.H + BS2D - 1) / BS2D, 1);
    trace_kernel<<<grid, block>>>(world, data, seed, frame);
}


