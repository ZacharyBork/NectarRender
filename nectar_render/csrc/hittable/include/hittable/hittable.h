#pragma once

#include <memory>
#include <vector>

#include "hittable/include/bvh/aabb.h"
#include "hittable/include/hittable/hit_record.h"
#include "material/include/material/material.h"
#include "engine/include/engine/transform.h"

// ============================================================================
// UTILS
// ============================================================================

struct HitTestResult { bool hit; HitRecord& rec; };

// ============================================================================
// BASE HITTABLE CLASS
// ============================================================================

class Hittable {
public:

    Transform xform, delta;
    Material* material = nullptr;
    AABB      bbox;

    Hittable* self_ref = this;

    /* CONSTRUCTORS */

    __host__ Hittable() : material(Lambertian(Color::purple()).build()) { }

    __host__ Hittable(
        const Vector3& position, 
        const Vector3& rotation,
        const Vector3& scale
    ) : xform(Transform(position, rotation, scale)),
        material(Lambertian(Color::purple()).build())
    { }

    __host__ Hittable(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale, 
        Material*      mat
    ) : xform(Transform(position, rotation, scale)), 
        material(mat) 
    { }

    template <typename M>
    __host__ Hittable(
        const Vector3& position, 
        const M&       material
    ) : xform(Transform(position)), material(material.build()) { }

    template <typename M>
    __host__ Hittable(const M& material) : material(material.build()) { }

    template <typename M>
    __host__ Hittable(
        const Vector3& position, 
        const Vector3& rotation,
        const Vector3& scale,
        const M&       material
    ) : xform(Transform(position, rotation, scale)),
        material(material.build())
    { }

    template <typename M>
    __host__ Hittable(
        const Vector3& position, 
        const Vector3& delta_position,
        const M&       material
    ) : xform(Transform(position)),
        delta(Transform(delta_position)),
        material(material.build())
    { }

    __device__ Hittable(
        Transform& xform, 
        Transform& delta, 
        Material*  mat
    ) : xform(xform), delta(delta), material(mat) { }

    __host__ virtual Hittable* build() const = 0;
    __host__ virtual const AABB build_bbox() const = 0;

    __host__ const AABB bounding_box() { 
        if (!bbox.bbox_empty) return bbox;
        return build_bbox(); 
    }

    __device__ const bool hit_test(
        const Ray& ray,
        HitRecord& rec
    ) const {
        HitRecord tmp_rec;
        Ray r = ray.to_object_space(xform);

        bool hit_obj = hit(r, Interval(EPS, FMAX), rec);
        if (hit_obj) rec.hit_object = self_ref;
        rec.to_world_space(xform, r, ray, true);
        
        return hit_obj;
    }

    __device__ virtual bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const = 0;

    __host__ void set_motion_vector(const Vector3& offset) {
        delta.set_position(offset);
    }

    __device__ Vector3 position_at_time(float time) const {
        if (delta.p().length() < 1e-16) return xform.p();
        Ray motion(xform.p(), delta.p());
        return motion.at(time);
    }

    __device__ virtual float pdf_value(
        const Vector3& origin,
        const Vector3& direction
    ) const {
        return 0.0f;
    }

    __device__ virtual Vector3 random(
        const Vector3& origin,
        Generator& gen
    ) const {
        return Vector3(1.0f, 0.0f, 0.0f);
    }

    __host__ void update_xform(Transform& xform);
    __host__ void update_material(Material* mat);
    
};

// ============================================================================
// KERNEL WRAPPERS
// ============================================================================

void run_update_xform(Hittable* hittable, Transform& xform);
__host__ inline void Hittable::update_xform(Transform& xform) {
    run_update_xform(this, xform);
}

void run_update_material(Hittable* hittable, Material* mat);
__host__ inline void Hittable::update_material(Material* mat) {
    run_update_material(this, mat);
}


