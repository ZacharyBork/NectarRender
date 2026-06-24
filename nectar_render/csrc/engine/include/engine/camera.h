#pragma once

#include <optional>
#include <array>

#include "core/include/core/utils.h"
#include "core/include/core/vector.h"
#include "core/include/core/matrix.h"
#include "core/include/core/constants.h"
#include "core/include/core/random.h"

#include "engine/include/engine/ray.h"

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

struct UVSample { 
    float u, v; 
    __host__ __device__ UVSample(float u, float v) : u(u), v(v) { }
};

class Camera {
public:
    Vector2 resolution;
    Vector3 position;
    Vector3 rotation;

    float focal_length;
    float focus_distance;
    float aperture;

    float sensor_width;
    float shutter_speed;

    __host__ explicit Camera(const CameraParams& p = {})
        : resolution(Vector2(p.resolution[0], p.resolution[1])),
          position(Vector3(p.position[0], p.position[1], p.position[2])),
          rotation(Vector3(p.rotation[0], p.rotation[1], p.rotation[2])),
          focal_length(p.focal_length),
          focus_distance(p.focus_distance),
          aperture(p.aperture),
          sensor_width(p.sensor_width),
          shutter_speed(p.shutter_speed)
    {
        shutter_time = 1.0f / (shutter_speed+FMIN);
        fstop = focal_length / (aperture+FMIN);

        float aperture_radius = aperture / 2.0;
        defocus_angle  = 2.0 * atanf(aperture_radius / (focus_distance+FMIN));
        defocus_radius = focus_distance * tanf(defocus_angle / 2.0);

        aspect_ratio = (float)resolution.y() / (float)resolution.x();
        uvw = Vector3(sensor_width, -sensor_width * aspect_ratio, 0.0f);
    }

    __device__ Ray spawn_rays(
        unsigned int x, 
        unsigned int y, 
        Generator& gen
    ) {
        Matrix3 R = rotation_from_euler(rotation);
        update_defocuse_disk(R);

        Vector2 offset = Vector2::sample_square(gen);
        float u = ((float)x + offset.x() - (resolution.x() - 1) * 0.5f) 
                * (uvw.x() / resolution.x());
        float v = ((float)y + offset.y() - (resolution.y() - 1) * 0.5f) 
                * (uvw.y() / resolution.y());
        float w = -focal_length;

        Vector3 local_dir = Vector3(u, v, w);
        Vector3 centers = R.transpose() * local_dir;

        float focus_scale = focus_distance / focal_length;
        Vector3 focus_points = position + centers * focus_scale;

        Vector3 origin;
        if (defocus_angle <= 0.0f) {
            origin = position;
        } else {
            Vector2 p = Vector2::random_in_unit_disk(gen);
            origin = position + (
                p.x() * defocus_disk_u + p.y() * defocus_disk_v
            );
        }

        Vector3 direction = focus_points - origin;
        return Ray(origin, direction);
    }

private:

    float fstop;
    float shutter_time;
    float defocus_angle;
    float defocus_radius;
    float aspect_ratio;

    Vector3 uvw;
    Vector3 defocus_disk_u;
    Vector3 defocus_disk_v;

    __device__ void update_defocuse_disk(Matrix3 R) {
        defocus_disk_u = R.right() * defocus_radius;
        defocus_disk_v = R.up()    * defocus_radius;
    }

    __device__ float build_ray_times(Generator& gen) {
        return gen.uniform() * shutter_time;
    }

    __device__ UVSample uv_sample(unsigned int x, unsigned int y) {
        return UVSample(
            ((float)x / (float)(resolution.x() - 1)),
            ((float)y / (float)(resolution.y() - 1))
        );
    }

};

