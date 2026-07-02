#pragma once

#include <array>

#include "core/include/core.h"
#include "engine/include/engine/ray.h"

// ############################################################################
// CAMERA PARAMETERS
// ############################################################################

struct CameraParams {
    std::array<int,   2> resolution = { 512, 512 };
    std::array<float, 3> position   = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> rotation   = { 0.0f, 0.0f, 0.0f };
    float focal_length   = 5.0f;
    float focus_distance = 10.0f;
    float aperture       = 0.01f;
    float sensor_width   = 2.0f;
    float shutter_speed  = 1.0f;
};

// ############################################################################
// DEVICE CAMERA
// ############################################################################

struct DeviceCamera {
public:

    CameraParams params;

    Vector2 resolution;
    Vector3 position;
    Matrix3 rotation;

    __host__ explicit DeviceCamera(
        const CameraParams& p = {}
    ) : params(p),
        resolution(Vector2(p.resolution[0], p.resolution[1])),
        position(Vector3(p.position[0], p.position[1], p.position[2]))
    {
        shutter_time = 1.0f / (p.shutter_speed + FMIN);
        float fstop = p.focal_length / (p.aperture + FMIN);

        float aperture_radius = p.aperture / 2.0f;
        defocus_angle  = 2.0 * atanf(
            aperture_radius / (p.focus_distance + FMIN)
        );
        defocus_radius = p.focus_distance * tanf(defocus_angle / 2.0);

        float aspect_ratio = (float)resolution.y() / (float)resolution.x();
        uvw = Vector3(p.sensor_width, -p.sensor_width * aspect_ratio, 0.0f);

        rotation = rotation_from_euler(
            Vector3(
                deg2rad(p.rotation[0]), 
                deg2rad(p.rotation[1]), 
                deg2rad(p.rotation[2])
            )
        );
    }

    __device__ Ray get_ray(uint32_t x, uint32_t y, Generator& gen) {
        Vector3 origin;
        Vector3 focus_point = get_focus_point(x, y, gen);
        
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
        Generator& gen
    ) {
        Vector2 offset = Vector2::sample_square(gen);
        float u = ((float)x + offset.x() - (resolution.x() - 1) * 0.5f) 
                * (uvw.x() / resolution.x());
        float v = ((float)y + offset.y() - (resolution.y() - 1) * 0.5f) 
                * (uvw.y() / resolution.y());
        float w = -params.focal_length;

        Vector3 center = rotation * Vector3(u, v, w);
        float focus_scale = params.focus_distance / params.focal_length;
        return position + center * focus_scale;
    }

private:

    float shutter_time;
    float defocus_angle;
    float defocus_radius;

    Vector3 uvw;
    Vector3 defocus_disk_u, defocus_disk_v;

};

// ############################################################################
// HOST CAMERA
// ############################################################################

class Camera {
public:

    CameraParams params;

    Vector2 resolution;

    __host__ explicit Camera(
        const CameraParams& p = {}
    ) : params(p), 
        resolution(Vector2(p.resolution[0], p.resolution[1]))
    { }

    __host__ DeviceCamera* device_camera() {
        DeviceCamera d_cam(params);
        DeviceCamera* d_ptr;
        size_t n_bytes = sizeof(DeviceCamera);
        
        cudaMalloc(&d_ptr, n_bytes);
        cudaMemcpy(d_ptr, &d_cam, n_bytes, cudaMemcpyHostToDevice);

        return d_ptr;
    }

};

