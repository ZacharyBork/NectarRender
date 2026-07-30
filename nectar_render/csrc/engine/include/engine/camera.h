#pragma once

#include <array>
#include <atomic>
#include <optional>
#include <algorithm>

#include "core/include/core.h"
#include "host/include/host/utils.h"
#include "random/include/hash.h"
#include "engine/include/engine/ray.h"

#include <pybind11/functional.h>

// ############################################################################
// SAMPLE GENERATION
// ############################################################################

class SampleGenerator {
public:
    __host__ void build(uint32_t n_samples, uint32_t seed) {
        if (n_samples == cached_n && seed == cached_seed) return;

        std::vector<Vector2> pattern = generate_n_rooks(n_samples, seed);

        if (d_pattern) CUDAMemory::free(d_pattern);
        CUDAMemory::allocate<Vector2>(d_pattern, n_samples);
        CUDAMemory::copy<Vector2>(d_pattern, pattern.data(), n_samples);

        cached_n    = n_samples;
        cached_seed = seed;
    }

    __host__ Vector2* pattern() const { return d_pattern; }
    __host__ bool has_pattern() const { return d_pattern != nullptr; }

    ~SampleGenerator() { if (d_pattern) CUDAMemory::free(d_pattern); }

private:

    Vector2* d_pattern   = nullptr;
    uint32_t cached_n    = 0;
    uint32_t cached_seed = 0;

    std::vector<Vector2> generate_n_rooks(uint32_t n, uint32_t seed) {
        std::vector<uint32_t> column_perm(n);
        std::iota(column_perm.begin(), column_perm.end(), 0);

        std::mt19937 rng(seed);
        std::shuffle(column_perm.begin(), column_perm.end(), rng);

        std::uniform_real_distribution<float> jitter(0.0f, 1.0f);
        std::vector<Vector2> samples(n);
        for (uint32_t i = 0; i < n; i++) {
            samples[i] = Vector2(
                (i               + jitter(rng)) / (float)n,
                (column_perm[i]  + jitter(rng)) / (float)n
            );
        }
        return samples;
    }
};

// ############################################################################
// PARAMETERS
// ############################################################################

struct CameraParams {
    Vector2  resolution;
    Vector3  position;
    Vector3  rotation;
    uint32_t samples_per_pixel;
    float    focal_length;
    float    focus_distance;
    float    aperture;
    float    sensor_width;
    float    shutter_speed;

    Matrix3 R;
    Vector3 uvw;
    float shutter_time;
    float aspect_ratio;
    float defocus_angle;
    float defocus_radius;

    float movement_speed   = 5.0f;
    float look_sensitivity = 10.0f;

    __host__ CameraParams() 
      : resolution(Vector2(0.0f)), 
        position(Vector3(0.0f)),
        rotation(Vector3(0.0f)),
        samples_per_pixel(0u), 
        focal_length(0.0f),
        focus_distance(0.0f), 
        aperture(0.0f), 
        sensor_width(0.0f),
        shutter_speed(0.0f)
    { }

    __host__ explicit CameraParams(
        Vector2  resolution,
        Vector3  position,
        Vector3  rotation,
        uint32_t samples_per_pixel,
        float    focal_length,
        float    focus_distance,
        float    aperture,
        float    sensor_width,
        float    shutter_speed
    ) : resolution        (resolution),
        position          (position),
        rotation          (rotation),
        samples_per_pixel (samples_per_pixel),
        focal_length      (focal_length),
        focus_distance    (focus_distance),
        aperture          (aperture),
        sensor_width      (sensor_width),
        shutter_speed     (shutter_speed)
    { build(); }

    __host__ __device__ void build() {
        R = rotation_from_euler(deg2rad(rotation));
        
        shutter_time = 1.0f / (shutter_speed + FMIN);
        float aperture_radius = aperture / 2.0f;
        
        defocus_angle = 2.0f * atanf(
            aperture_radius / (focus_distance + FMIN)
        );
        defocus_radius = focus_distance * tanf(defocus_angle / 2.0f);
        float aspect_ratio = resolution.y() / resolution.x();
        uvw = Vector3(sensor_width, -sensor_width * aspect_ratio, 0.0f);
    }

    __host__ void update(const CameraParams& other) {
        Vector3 pos = other.position * movement_speed;
        Vector3 rot = other.rotation * look_sensitivity;
        
        rotation[1] -= rot[1];
        rotation[0] -= rot[0];
        rotation[0]  = fmaxf(-89.0f, fminf(89.0f, rotation[0]));

        R = rotation_y(deg2rad(rotation[1])) 
          * rotation_x(deg2rad(rotation[0]));

        position += R * pos;

        focal_length   = other.focal_length;
        focus_distance = other.focus_distance;
        aperture       = other.aperture;
        sensor_width   = other.sensor_width;
        shutter_speed  = other.shutter_speed;

        build();
    }

};

// ############################################################################
// DEVICE CAMERA
// ############################################################################

struct DeviceCamera {
public:

    CameraParams p;

    __host__ explicit DeviceCamera(
        CameraParams params,
        Vector2* sample_pattern
    ) : p(params), sample_pattern(sample_pattern) { }

    __device__ Ray get_ray(
        uint32_t x,
        uint32_t y,
        uint32_t pixel_idx,
        uint32_t sample_idx,
        uint32_t seed,
        Generator& gen
    ) {
        Vector3 origin;
        Vector3 focus_point = get_focus_point(
            x, y, pixel_idx, sample_idx, seed
        );

        if (p.defocus_angle <= 0.0f) origin = p.position;
        else {
            update_defocuse_disk();
            Vector2 loc = Vector2::random_in_unit_disk(gen);
            Vector3 defocus = loc.x() * defocus_disk_u 
                            + loc.y() * defocus_disk_v;

            origin = p.position + defocus;
        }

        float time = (
            p.shutter_time > 0.0f ? gen.uniform() * p.shutter_time : 0.0f
        );
        return Ray(origin, normalize(focus_point - origin), time);
    }

    __device__ Ray screen_space_ray(float su, float sv) {
        float x = su * p.resolution.x();
        float y = sv * p.resolution.y();

        float u = (x - (p.resolution.x() - 1.0f) * 0.5f) 
                * (p.uvw.x() / p.resolution.x());
        
                float v = (y - (p.resolution.y() - 1.0f) * 0.5f) 
                * (p.uvw.y() / p.resolution.y());

        Vector3 direction = normalize(p.R * Vector3(u, v, -p.focal_length));
        return Ray(p.position, direction);
    }

    __host__ __device__ void set_sample_pattern(Vector2* pattern) { 
        sample_pattern = pattern; 
    }

private:

    Vector2* sample_pattern;
    Vector3 defocus_disk_u, defocus_disk_v;

    __device__ void update_defocuse_disk() {
        defocus_disk_u = p.R.right() * p.defocus_radius;
        defocus_disk_v = p.R.up()    * p.defocus_radius;
    }

    __device__ float build_ray_time(Generator& gen) {
        return gen.uniform() * p.shutter_time;
    }

    __device__ Vector2 sample_offset(
        uint32_t sample_i, 
        uint32_t pixel_idx, 
        uint32_t render_seed
    ) const {
        Vector2 base  = sample_pattern[sample_i];
        Vector2 shift = pcg_vector2(pixel_idx, render_seed);

        return Vector2(
            fmodf(base.x() + shift.x(), 1.0f),
            fmodf(base.y() + shift.y(), 1.0f)
        ) - 0.5f;
    }

    __device__ Vector3 get_focus_point(
        uint32_t x, 
        uint32_t y, 
        uint32_t pixel_idx, 
        uint32_t sample_idx, 
        uint32_t render_seed
    ) {
        Vector2 offset = sample_offset(sample_idx, pixel_idx, render_seed);
        float u = ((float)x + offset.x() - (p.resolution.x() - 1.0f) * 0.5f)
                * (p.uvw.x() / p.resolution.x());
        float v = ((float)y + offset.y() - (p.resolution.y() - 1.0f) * 0.5f)
                * (p.uvw.y() / p.resolution.y());
        float w = -p.focal_length;

        Vector3 center = p.R * Vector3(u, v, w);
        float focus_scale = p.focus_distance / p.focal_length;
        return p.position + center * focus_scale;
    }

};

// ############################################################################
// HOST CAMERA
// ############################################################################

class Camera {
public:

    std::function<void(CameraParams)> on_updated = hook_no_op<CameraParams>;

    __host__ explicit Camera(CameraParams p) : p(p) { }

    __host__ Camera(const Camera& other) : Camera(other.p) { }

    __host__ ~Camera() { if (d_cam_ptr) CUDAMemory::free(d_cam_ptr); }

    Camera& operator=(const Camera&) = delete;

    __host__ void __construct(const uint32_t seed) {
        if (seed_ != seed || !sample_generator.has_pattern()) {
            sample_generator.build(p.samples_per_pixel, seed);
        }
        seed_ = seed;

        DeviceCamera d_cam(p, sample_generator.pattern());        
        if (!d_cam_ptr) { CUDAMemory::allocate<DeviceCamera>(d_cam_ptr); }
        CUDAMemory::copy<DeviceCamera>(d_cam_ptr, &d_cam);
    }

    __host__ void update(CameraParams params) {
        p.update(params);
        with_gil_scoped_acquire(on_updated, p);
    }

    __host__ DeviceCamera* device_camera() {
        if (!d_cam_ptr)
            throw std::runtime_error(
                "Camera::device_camera() called before Camera::_construct(). "
                "Camera::d_cam_ptr is null.");
        return d_cam_ptr;
    }

    __host__ Ray screen_space_ray(float su, float sv) {
        float rx = p.resolution.x();
        float ry = p.resolution.y();

        float x = su * rx;
        float y = sv * ry;

        float u = (x - (rx - 1.0f) * 0.5f) * (p.uvw.x() / rx);
        float v = (y - (ry - 1.0f) * 0.5f) * (p.uvw.y() / ry);

        Vector3 direction = normalize(p.R * Vector3(u, v, -p.focal_length));
        return Ray(p.position, direction);
    }

    __host__ std::optional<Vector2> project_to_screen(
        const Vector3& world_point
    ) const {
        Vector3 local = p.R.T() * (world_point - p.position);
        if (local.z() >= 0.0f) return std::nullopt;

        float scale = -p.focal_length / local.z();
        float lu = local.x() * scale;
        float lv = local.y() * scale;

        float rx = p.resolution.x();
        float ry = p.resolution.y();

        float px = lu * (rx / p.uvw.x()) + (rx - 1.0f) * 0.5f;
        float py = lv * (ry / p.uvw.y()) + (ry - 1.0f) * 0.5f;

        return Vector2(px / rx, py / ry);
    }

    __host__ const Vector2 resolution() const { return p.resolution; }
    __host__ uint32_t n_samples() const { return p.samples_per_pixel; }
    
    __host__ CameraParams* parameters_() { return &p; }
    __host__ const CameraParams parameters() const { return p; }

    template<typename T>
    __host__ T* make_buffer(size_t channels) {
        T* d_buffer_ptr;
        Vector2 r = resolution();
        size_t n_elements = channels * (size_t)r.x() * (size_t)r.y();
        CUDAMemory::allocate<T>(d_buffer_ptr, n_elements);
        return d_buffer_ptr;
    }
        
private:

    CameraParams p;
    SampleGenerator sample_generator;
    DeviceCamera* d_cam_ptr = nullptr;

    uint32_t seed_ = 0u;

    __host__ void free_device_pointer() {
        if (d_cam_ptr) CUDAMemory::free(d_cam_ptr);
        d_cam_ptr = nullptr;
    }

};

