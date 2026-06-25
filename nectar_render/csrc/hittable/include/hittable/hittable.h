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

#include <iostream>

class Hittable {
public:

    Vector3   position, delta_position;
    Material* material;

    __host__ Hittable()
        : position(Vector3(0.0f, 0.0f, 0.0f)), 
          material(Lambertian(Color(1.0f, 0.0f, 1.0f)).build()) 
    { }

    template <typename M>
    __host__ Hittable(const Vector3& position, const M& material)
        : position(position), material(material.build()) 
    { }

    __device__ Hittable(const Vector3& pos, Material* mat) {
        position = pos;
        material = mat;
    }

    __host__ __device__ virtual ~Hittable() = default;    
    __host__ virtual Hittable* build() const = 0;

    __device__ virtual bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const = 0;

    __host__ void set_motion_vector(const Vector3& offset) {
        delta_position = offset;
    }

    __device__ Vector3 position_at_time(float time) const {
        // if (delta_position.length() < 1e-16) return position;
        Ray motion(position, delta_position);
        return motion.at(time);
    }
};

// ============================================================================
// SHAPES
// ============================================================================

class Sphere : public Hittable {
public:
    
    template <typename M>
    __host__ Sphere(const Vector3& position, float radius, const M& material) 
        : Hittable(position, material), radius(radius + FMIN) { }

    __device__ Sphere(const Vector3& pos, float rad, Material* mat) 
        : Hittable(pos, mat), radius(rad) {}

    __host__ Hittable* build() const override {
        return device_build<Sphere>(position, radius, material);
    }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override {
        Ray motion(position, delta_position);
        Vector3 current_position =  motion.at(ray.time());
        // Vector3 current_position = position_at_time(ray.time());

        Vector3 oc = current_position - ray.origin();
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
        rec.material = material;
        Vector3 norm = (rec.position - current_position) / (radius + FMIN);
        rec.set_face_normal(ray, norm);

        return true;
    }

private:

    float radius;
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

