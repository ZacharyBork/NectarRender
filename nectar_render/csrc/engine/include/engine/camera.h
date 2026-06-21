#pragma once

#include "core/include/core/vector.h"

struct CameraParams {
    int res_x = 512;
    int res_y = 512;
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 rotation = Vector3(0.0f, 0.0f, 0.0f);
    float focal_length   = 5.0f;
    float focus_distance = 10.0f;
    float aperture       = 1.0f;
    float sensor_width   = 2.0f;
    float shutter_speed  = 1.0f;
};

class Camera {
public:
    int resolution[2];
    Vector3 position;
    Vector3 rotation;

    float focal_length;
    float focus_distance;
    float aperture;

    float sensor_width;
    float shutter_speed;

    explicit Camera(const CameraParams& p = {})
        : resolution({p.res_x, p.res_y}),
          position(p.position),
          rotation(p.rotation),
          focal_length(p.focal_length),
          focus_distance(p.focus_distance),
          aperture(p.aperture),
          sensor_width(p.sensor_width),
          shutter_speed(p.shutter_speed)
    {

    }

private:

    float fstop;
    float defocus_angle;
    float defocus_radius;
    float aspect_ratio;

    Vector3 defocus_disk_u;
    Vector3 defocus_disk_v;


};

