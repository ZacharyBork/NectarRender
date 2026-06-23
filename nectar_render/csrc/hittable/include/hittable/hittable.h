#pragma once

#include <memory>
#include <vector>

#include "core/include/core/vector.h"
#include "core/include/core/interval.h"
#include "engine/include/engine/ray.h"

template<typename T, typename... Args>
T* device_build(Args... args);

// ============================================================================
// HIT RECORD
// ============================================================================

class HitRecord {
public:
    Point3  position;
    Vector3 normal;
    float   t;
    bool    front_face;

    __device__ void set_face_normal(
        const Ray& ray, 
        const Vector3& outward_normal
    ) {
        front_face = dot(ray.direction(), outward_normal) < 0.0f;
        normal     = front_face ? outward_normal : -outward_normal;
    }
};

// ============================================================================
// ABSTRACT PARENT
// ============================================================================

class Hittable {
public:
    __host__ __device__ virtual ~Hittable() = default;

    __device__ virtual bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const = 0;

    virtual Hittable* build() const = 0;
};

// ============================================================================
// SHAPES
// ============================================================================

class Sphere : public Hittable {
public:
    __host__ __device__ Sphere(const Point3& center, float radius) 
        : center(center), radius(fmax(0.0f, radius)) {}

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override {
        Vector3 oc = center - ray.origin();
        float a = ray.direction().length_squared();
        float h = dot(ray.direction(), oc);
        float c = oc.length_squared() - radius * radius;
        
        float discriminant = h * h - a * c;
        if (discriminant < 0) return false;

        auto sqrtd = sqrtf(discriminant);

        auto root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t        = root;
        rec.position = ray.at(rec.t);
        rec.normal   = (rec.position - center) / radius;
        Vector3 norm = (rec.position - center) / radius;
        rec.set_face_normal(ray, norm);

        return true;
    }

    Hittable* build() const override {
        return device_build<Sphere, Point3, float>(center, radius);
    }

private:
    Point3 center;
    float  radius;
};

// ============================================================================
// HITTABLES LIST
// ============================================================================

struct HittablesList {
    Hittable** objects;
    int        n_objects;

    __device__ bool hit(
        const Ray& ray,
        Interval   ray_t,
        HitRecord& rec
    ) const {
        HitRecord temp_rec;
        bool  hit_anything = false;
        float closest      = ray_t.max;

        for (int i = 0; i < n_objects; i++) {
            if (objects[i]->hit(ray, Interval(ray_t.min, closest), temp_rec)) {
                hit_anything = true;
                closest      = temp_rec.t;
                rec          = temp_rec;
            }
        }
        return hit_anything;
    }
};

