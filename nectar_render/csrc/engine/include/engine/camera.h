#pragma once

#include <array>
#include <algorithm>

#include "core/include/core.h"
#include "random/include/hash.h"
#include "engine/include/engine/ray.h"

// ############################################################################
// SAMPLE GENERATION
// ############################################################################

class SampleGenerator {
public:
    __host__ void build(uint32_t n_samples, uint32_t seed) {
        if (n_samples == cached_n && seed == cached_seed) return;

        std::vector<Vector2> pattern = generate_n_rooks(n_samples, seed);

        if (d_pattern) cudaFree(d_pattern);
        cudaMalloc(&d_pattern, n_samples * sizeof(Vector2));
        cudaMemcpy(
            d_pattern, pattern.data(), 
            n_samples * sizeof(Vector2), 
            cudaMemcpyHostToDevice
        );

        cached_n    = n_samples;
        cached_seed = seed;
    }

    __host__ Vector2* pattern() const { return d_pattern; }

    ~SampleGenerator() { if (d_pattern) cudaFree(d_pattern); }

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
// DEVICE CAMERA
// ############################################################################

struct DeviceCamera {
public:

    Vector2 resolution;
    Vector3 position;
    Matrix3 rotation;

    __host__ explicit DeviceCamera(
        const Vector2 resolution,
        const Vector3 position,
        const Matrix3 rotation,
        Vector2* sample_pattern,
        float focal_length,
        float focus_distance,
        float shutter_time,
        float defocus_angle,
        float defocus_radius,
        const Vector3 uvw
    ) : resolution(resolution),
        position(position),
        rotation(rotation),
        sample_pattern(sample_pattern),
        focal_length(focal_length),
        focus_distance(focus_distance),
        shutter_time(shutter_time),
        defocus_angle(defocus_angle),
        defocus_radius(defocus_radius),
        uvw(uvw)
    { }

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

        if (defocus_angle <= 0.0f) origin = position;
        else {
            update_defocuse_disk();
            Vector2 p = Vector2::random_in_unit_disk(gen);
            Vector3 defocus = p.x() * defocus_disk_u + p.y() * defocus_disk_v;
            origin = position + defocus;
        }

        float time = shutter_time > 0.0f ? gen.uniform() * shutter_time : 0.0f;
        return Ray(origin, normalize(focus_point - origin), time);
    }

    __host__ __device__ void set_sample_pattern(Vector2* pattern) { 
        sample_pattern = pattern; 
    }

private:

    Vector2* sample_pattern;

    float focal_length;
    float focus_distance;
    float shutter_time;
    float defocus_angle;
    float defocus_radius;

    Vector3 uvw;
    Vector3 defocus_disk_u, defocus_disk_v;

    __device__ void update_defocuse_disk() {
        defocus_disk_u = rotation.right() * defocus_radius;
        defocus_disk_v = rotation.up()    * defocus_radius;
    }

    __device__ float build_ray_time(Generator& gen) {
        return gen.uniform() * shutter_time;
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
        float u = ((float)x + offset.x() - (resolution.x() - 1.0f) * 0.5f)
                * (uvw.x() / resolution.x());
        float v = ((float)y + offset.y() - (resolution.y() - 1.0f) * 0.5f)
                * (uvw.y() / resolution.y());
        float w = -focal_length;

        Vector3 center = rotation * Vector3(u, v, w);
        float focus_scale = focus_distance / focal_length;
        return position + center * focus_scale;
    }

};

// ############################################################################
// HOST CAMERA
// ############################################################################

class Camera {
public:

    Vector2 resolution;
    Vector3 position;
    Matrix3 rotation;
    
    uint32_t n_samples;

    float focal_length;
    float focus_distance;
    float aperture;
    float sensor_width;
    float shutter_speed;

    __host__ explicit Camera(
        std::array<int,   2> resolution = { 512, 512 },
        std::array<float, 3> position   = { 0.0f, 0.0f, 0.0f },
        std::array<float, 3> rotation   = { 0.0f, 0.0f, 0.0f },
        uint32_t samples_per_pixel = 500u,
        float focal_length   = 5.0f,
        float focus_distance = 10.0f,
        float aperture       = 0.01f,
        float sensor_width   = 2.0f,
        float shutter_speed  = 1.0f
    ) : resolution     (Vector2(resolution)),
        position       (Vector3(position)),
        pitch_         (rotation[0]),
        yaw_           (rotation[1]),
        roll_          (rotation[2]),
        rotation       (rotation_from_euler(deg2rad(Vector3(rotation)))),
        n_samples      (samples_per_pixel),
        focal_length   (focal_length),
        focus_distance (focus_distance),
        aperture       (aperture),
        sensor_width   (sensor_width),
        shutter_speed  (shutter_speed)
    { }

    __host__ Camera(const Camera& other) 
      : resolution     (other.resolution),
        position       (other.position),
        rotation       (other.rotation),
        n_samples      (other.n_samples),
        focal_length   (other.focal_length),
        focus_distance (other.focus_distance),
        aperture       (other.aperture),
        sensor_width   (other.sensor_width),
        shutter_speed  (other.shutter_speed)
    { }

    __host__ ~Camera() { free_device_pointer(); }

    Camera& operator=(const Camera&) = delete;

    __host__ void __construct(const uint32_t seed) {
        seed_ = seed;
        free_device_pointer();
        sample_generator.build(n_samples, seed_);

        DeviceCamera d_cam = build_device_camera();
        size_t n_bytes = sizeof(d_cam);
        
        cudaMalloc(&d_cam_ptr, n_bytes);
        cudaMemcpy(d_cam_ptr, &d_cam, n_bytes, cudaMemcpyHostToDevice);
    }

    __host__ DeviceCamera* device_camera() {
        if (!d_cam_ptr)
            throw std::runtime_error(
                "Camera::device_camera() called before Camera::_construct(). "
                "Camera::d_cam_ptr is null.");
        return d_cam_ptr;
    }

    __host__ void update(
        const Vector3& delta_position,
        const Vector3& delta_rotation,
        float focal_length_
    ) {
        cudaDeviceSynchronize();
        
        yaw_   -= delta_rotation.y();
        pitch_ -= delta_rotation.x();
        pitch_  = fmaxf(-89.0f, fminf(89.0f, pitch_));

        rotation = rotation_y(deg2rad(yaw_)) * rotation_x(deg2rad(pitch_));

        position += rotation * delta_position;

        focal_length = focal_length_;
        __construct(seed_);
    }

private:

    DeviceCamera* d_cam_ptr = nullptr;
    SampleGenerator sample_generator;

    uint32_t seed_ = 0u;

    float pitch_ = 0.0f;
    float yaw_   = 0.0f;
    float roll_  = 0.0f;

    __host__ void free_device_pointer() {
        if (d_cam_ptr) 
            cudaFree(reinterpret_cast<void*>(d_cam_ptr));
        d_cam_ptr = nullptr;
    }

    __host__ DeviceCamera build_device_camera() {
        float shutter_time    = 1.0f / (shutter_speed + FMIN);
        float aperture_radius = aperture / 2.0f;
        
        float defocus_angle = 2.0f * atanf(
            aperture_radius / (focus_distance + FMIN)
        );
        float defocus_radius = focus_distance * tanf(defocus_angle / 2.0f);
        float aspect_ratio   = resolution.y() / resolution.x();
        
        Vector3 uvw(sensor_width, -sensor_width * aspect_ratio, 0.0f);
         
        return DeviceCamera(
            resolution, position, rotation, sample_generator.pattern(), 
            focal_length, focus_distance, shutter_time, defocus_angle, 
            defocus_radius, uvw
        );
    }

};

