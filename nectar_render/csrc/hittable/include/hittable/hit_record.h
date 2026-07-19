#pragma once

#include "core/include/core.h"
#include "engine/include/engine/ray.h"

class Hittable;
class Material;

class HitRecord {
public:

    Hittable* hit_object  = nullptr;
    uint32_t  object_index = 0u;

    Vector3 p;               // Position
    Vector3 n;               // Surface normal vector
    Vector3 tangent;         // Surface tangent vector
    Vector2 uv;              // UVs
    float   t;               // Distance
    bool    front_face;      // Front / Back face
    Material* mat = nullptr; // Material reference

    __device__ void to_world_space(
        const Transform& xform,
        const Ray& object_ray,
        const Ray& world_ray,
        bool apply_bias = false
    ) {
        p = xform.R() * (p * xform.scale()) + xform.pos();
        tangent = normalize(xform.R() * (tangent * xform.scale()));
        
        front_face = dot(object_ray.direction(), n) < 0.0f;
        n = front_face ? n : -n;
        n = normalize(xform.R() * (n * xform.inv_s()));
        
        front_face = dot(world_ray.direction(), n) < 0.0f;
        n = front_face ? n : -n;
        
        tangent = normalize(tangent - n * dot(tangent, n));
        if (tangent.near_zero()) 
        tangent = normalize(
            cross(n, fabsf(n.x()) > 0.9f ? 
            Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f))
        );
        if (apply_bias) p += n * EPS;

        t = dot(
            p - world_ray.origin(), world_ray.direction()
        ) / world_ray.direction().length_squared();
    }

    __host__ uintptr_t d_object_ptr() const {
        return reinterpret_cast<uintptr_t>(hit_object);
    }

};

