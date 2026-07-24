#pragma once

#include <memory>
#include <vector>

#include "hittable/include/bvh/aabb.h"
#include "hittable/include/hittable/hit_record.h"
#include "material/include/material/material.h"
#include "core/include/core/transform.h"

// ============================================================================
// UTILS
// ============================================================================

struct HitTestResult { bool hit; HitRecord& rec; };

// ============================================================================
// BASE HITTABLE CLASS
// ============================================================================

struct MatRegistryView;

class Hittable {
public:

    Hittable* self_ref = this;
    Transform xform, delta;
    AABB      bbox;

    Material material;
    size_t   material_index = (size_t)0;
    size_t   object_index   = (size_t)0;

    /* CONSTRUCTORS */

    __host__ Hittable() : material(Material::lambertian(Color::purple())) { }
    __host__ Hittable(Material material) : material(std::move(material)) { }

    __host__ Hittable(const Vector3& position, Material material) 
        : xform(Transform(position)), material(std::move(material)) { }

    __host__ Hittable(
        const Vector3& position, 
        const Vector3& rotation,
        const Vector3& scale
    ) : xform(Transform(position, rotation, scale)),
        material(Material::lambertian(Color::purple()))
    { }

    __host__ Hittable(
        const Vector3&  position, 
        const Vector3&  rotation,
        const Vector3&  scale,
        Material material
    ) : xform(Transform(position, rotation, scale)),
        material(std::move(material))
    { }

    __host__ Hittable(
        const Vector3&  position, 
        const Vector3&  delta_position,
        Material material
    ) : xform(Transform(position)),
        delta(Transform(delta_position)),
        material(std::move(material))
    { }

    __host__ Hittable(
        Transform& xform, 
        Transform& delta, 
        Material material
    ) : xform(xform), delta(delta), material(std::move(material)) { }

    __device__ Hittable(
        Transform& xform, 
        Transform& delta, 
        size_t material_index
    ) : xform(xform), delta(delta), material_index(material_index) { }

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


