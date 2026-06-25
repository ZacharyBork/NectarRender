#pragma once

#include <memory>
#include <vector>

#include "hittable/include/bvh/aabb.h"
#include "hittable/include/hittable/hit_record.h"
#include "material/include/material/material.h"

// ============================================================================
// ABSTRACT PARENT
// ============================================================================

class Hittable {
public:

    Vector3   position, delta_pos;
    Material* material = nullptr;
    AABB      bbox;

    __host__ __device__ virtual ~Hittable() = default;

    __host__ Hittable()
        : position(Vector3(0.0f, 0.0f, 0.0f)), 
          material(Lambertian(Color(1.0f, 0.0f, 1.0f)).build())
    { }

    template <typename M>
    __host__ Hittable(
        const Vector3& position, 
        const M& material
    ) : position(position), 
        material(material.build())
    { }

    __device__ Hittable(
        const Vector3& pos, 
        const Vector3& delta, 
        Material* mat
    ) {
        position  = pos;
        delta_pos = delta;
        material  = mat;
    }

    __host__ virtual Hittable* build() const = 0;
    __host__ virtual const AABB build_bbox() const = 0;

    __host__ const AABB bounding_box() { 
        if (!bbox.bbox_empty) return bbox;
        return build_bbox(); 
    }

    __device__ virtual bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const = 0;

    __host__ void set_motion_vector(const Vector3& offset) {
        delta_pos = offset;
    }

    __device__ Vector3 position_at_time(float time) const {
        if (delta_pos.length() < 1e-16) return position;
        Ray motion(position, delta_pos);
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

    __device__ Sphere(
        const Vector3& pos, 
        const Vector3& delta,  
        float rad, 
        Material* mat
    ) : Hittable(pos, delta, mat), radius(rad) {}

    __host__ Hittable* build() const override {
        return device_build<Sphere>(
            position, delta_pos, radius, material
        );
    }

    __host__ const AABB build_bbox() const override {
        Vector3 rvec = Vector3(radius, radius, radius);
        return AABB(position - rvec, position + rvec);
    }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override {
        Vector3 current_position = position_at_time(ray.time());

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

