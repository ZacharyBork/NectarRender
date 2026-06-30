#pragma once

#include "core/include/core/vector.h"
#include "engine/include/engine/transform.h"
#include "engine/include/engine/ray.h"

class Material;

class HitRecord {
public:
    Vector3 p;               // Position
    Vector3 n;               // Normals
    Vector2 uv;              // UVs
    float   t;               // Distance
    bool    front_face;      // Front / Back face
    Material* mat = nullptr; // Material reference

    __device__ void set_face_normal(
        const Ray& ray, 
        const Vector3& outward_normal
    ) {
        front_face = dot(ray.direction(), outward_normal) < 0.0f;
        n = front_face ? outward_normal : -outward_normal;
    }

    __device__ void to_world_space(
        const Transform& xform,
        const Ray& object_ray,
        const Ray& world_ray
    ) {
        p = xform.R() * (p * xform.scale()) + xform.pos();
        
        front_face = dot(object_ray.direction(), n) < 0.0f;
        n = front_face ? n : -n;
        n = normalize(xform.R() * (n * xform.inv_s()));
        
        front_face = dot(world_ray.direction(), n) < 0.0f;
        n  = front_face ? n : -n;
        p += n * EPS;

        t = dot(
            p - world_ray.origin(), world_ray.direction()
        ) / world_ray.direction().length_squared();
    }

};

