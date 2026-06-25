#pragma once

#include <memory>
#include <vector>

#include "core/include/core/vector.h"
#include "core/include/core/interval.h"
#include "engine/include/engine/ray.h"
#include "hittable/include/hittable/hit_record.h"
#include "material/include/material/material.h"

template<typename T, typename... Args>
T* device_build(Args... args);

// ============================================================================
// ABSTRACT PARENT
// ============================================================================

class Hittable {
public:

    __host__ __device__ virtual ~Hittable() = default;
    __host__ virtual Hittable* build() const = 0;

    __device__ virtual bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const = 0;
};

// ============================================================================
// SHAPES
// ============================================================================

class Sphere : public Hittable {
public:
    
    template <typename M>
    __host__ Sphere(
        const Vector3& center, float radius, const M& material
    ) : center(center), radius(radius + FMIN), mat(material.build()) { }

    __device__ Sphere(const Vector3& center, float radius, Material* mat)
        : center(center), radius(radius), mat(mat) {}

    __host__ Hittable* build() const override {
        return device_build<Sphere>(center, radius, mat);
    }

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
        rec.material = mat;
        Vector3 norm = (rec.position - center) / (radius + FMIN);
        rec.set_face_normal(ray, norm);

        return true;
    }

private:
    Vector3   center;
    float     radius;
    Material* mat;
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

