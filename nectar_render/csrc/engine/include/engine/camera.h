#pragma once

#include "core/include/core/utils.h"
#include "core/include/core/vector.h"
#include "core/include/core/matrix.h"
#include "core/include/core/constants.h"
#include "core/include/core/random.h"

#include "engine/include/engine/ray.h"

struct CameraParams {
    Vector2 resolution = Vector2(512, 512);
    Vector3 position   = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 rotation   = Vector3(0.0f, 0.0f, 0.0f);
    float focal_length   = 5.0f;
    float focus_distance = 10.0f;
    float aperture       = 1.0f;
    float sensor_width   = 2.0f;
    float shutter_speed  = 1.0f;
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

    explicit Camera(const CameraParams& p = {})
        : resolution(p.resolution),
          position(p.position),
          rotation(p.rotation),
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
        defocus_radius = focus_distance * atanf(defocus_angle / 2.0);

        aspect_ratio = (float)resolution.x() / (float)resolution.y();
        uvw = Vector3(sensor_width, -sensor_width * aspect_ratio, 0.0);
    }

    Ray spawn_rays(Generator& gen) {
        float ray_time = build_ray_times(gen);


        
    }

private:

    float shutter_time;

    float fstop;
    float defocus_angle;
    float defocus_radius;
    float aspect_ratio;

    Vector3 uvw;
    Vector3 defocus_disk_u;
    Vector3 defocus_disk_v;

    float build_ray_times(Generator& gen) {
        return gen.uniform() * shutter_time;
    }


};

