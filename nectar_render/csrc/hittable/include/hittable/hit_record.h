#pragma once

#include "core/include/core/vector.h"
#include "engine/include/engine/ray.h"

class Material;

class HitRecord {
public:
    Vector3 position;
    Vector3 normal;
    float   t;
    bool    front_face;

    Material* material = nullptr;

    __device__ void set_face_normal(
        const Ray& ray, 
        const Vector3& outward_normal
    ) {
        front_face = dot(ray.direction(), outward_normal) < 0.0f;
        normal     = front_face ? outward_normal : -outward_normal;
    }
};

