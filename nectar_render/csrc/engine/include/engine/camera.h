#pragma once

#include <array>

#include "core/include/core.h"
#include "engine/include/engine/ray.h"

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
        float rsqrt_n_samples,
        float focal_length,
        float focus_distance,
        float shutter_time,
        float defocus_angle,
        float defocus_radius,
        const Vector3 uvw
    ) : resolution(resolution),
        position(position),
        rotation(rotation),
        rsqrt_n_samples(rsqrt_n_samples),
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
        uint32_t s_x,
        uint32_t s_y,  
        Generator& gen
    ) {
        Vector3 origin;
        Vector3 focus_point = get_focus_point(x, y, s_x, s_y, gen);
        
        if (defocus_angle <= 0.0f) origin = position; 
        else {
            update_defocuse_disk();
            Vector2 p = Vector2::random_in_unit_disk(gen);
            origin = position + (
                p.x() * defocus_disk_u + p.y() * defocus_disk_v
            );
        }

        float time = shutter_time > 0.0f ? gen.uniform() * shutter_time : 0.0f;
        return Ray(origin, normalize(focus_point - origin), time);
    }

private:

    float rsqrt_n_samples;

    float focal_length;
    float focus_distance;
    float shutter_time;
    float defocus_angle;
    float defocus_radius;

    Vector3 uvw;
    Vector3 defocus_disk_u, defocus_disk_v;

    __device__ const Vector2 sample_square_stratified(
        uint32_t s_x,
        uint32_t s_y,
        Generator& gen
    ) {
        float px = (((float)s_x + gen.random_float()) * rsqrt_n_samples);
        float py = (((float)s_y + gen.random_float()) * rsqrt_n_samples);
        return Vector2(px - 0.5f, py - 0.5f);
    }

    __device__ void update_defocuse_disk() {
        defocus_disk_u = rotation.right() * defocus_radius;
        defocus_disk_v = rotation.up()    * defocus_radius;
    }

    __device__ float build_ray_time(Generator& gen) {
        return gen.uniform() * shutter_time;
    }

    __device__ Vector3 get_focus_point(
        uint32_t x, 
        uint32_t y, 
        uint32_t s_x,
        uint32_t s_y,
        Generator& gen
    ) {
        Vector2 offset = sample_square_stratified(s_x, s_y, gen);
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

#include <iostream>

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
    ) : resolution(Vector2(resolution)),
        position(Vector3(position)),
        rotation(rotation_from_euler(deg2rad(Vector3(rotation)))),
        n_samples(samples_per_pixel),
        focal_length(focal_length),
        focus_distance(focus_distance),
        aperture(aperture),
        sensor_width(sensor_width),
        shutter_speed(shutter_speed)
    { }

    __host__ uint32_t sqrt_n_samples() {
        return (uint32_t)sqrtf((float)n_samples);
    }

    __host__ void __construct() {
        get_true_sample_count();
        free_device_pointer();
        
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

private:

    DeviceCamera* d_cam_ptr = nullptr;

    __host__ void free_device_pointer() {
        if (d_cam_ptr != nullptr)
            cudaFree(reinterpret_cast<void*>(d_cam_ptr));
            d_cam_ptr = nullptr;
    }

    __host__ void get_true_sample_count() {
        uint32_t sqrt_ns = (uint32_t)sqrtf((float)n_samples);
        n_samples = sqrt_ns * sqrt_ns;
    }

    __host__ DeviceCamera build_device_camera() {
        float rsqrt_n_samples = 1.0f / (float)sqrt_n_samples(); 
        float shutter_time    = 1.0f / (shutter_speed + FMIN);
        float aperture_radius = aperture / 2.0f;
        
        float defocus_angle   = 2.0f * atanf(
            aperture_radius / (focus_distance + FMIN)
        );
        float defocus_radius = focus_distance * tanf(defocus_angle / 2.0f);
        float aspect_ratio   = resolution.y() / resolution.x();
        
        Vector3 uvw(sensor_width, -sensor_width * aspect_ratio, 0.0f);
         
        return DeviceCamera(
            resolution, position, rotation, rsqrt_n_samples, focal_length, 
            focus_distance, shutter_time, defocus_angle, defocus_radius, uvw
        );
    }

};

