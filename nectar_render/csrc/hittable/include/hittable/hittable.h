#pragma once

#include <memory>
#include <vector>

#include "hittable/include/bvh/aabb.h"
#include "hittable/include/hittable/hit_record.h"
#include "material/include/material/material.h"
#include "engine/include/engine/transform.h"

// ============================================================================
// ABSTRACT PARENT
// ============================================================================

class Hittable {
public:

    Transform xform, delta;
    Material* material = nullptr;
    AABB      bbox;

    __host__ __device__ virtual ~Hittable() = default;

    __host__ Hittable() : material(Lambertian(Color::purple()).build()) { }

    __host__ Hittable(
        const Vector3& position, 
        const Vector3& rotation,
        const Vector3& scale
    ) : xform(Transform(position, rotation, scale)),
        material(Lambertian(Color::purple()).build())
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

    template <typename M>
    __host__ Hittable(
        Transform& xform, 
        Transform& delta, 
        const M&   material
    ) : xform(xform), delta(delta), material(material) { }

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
};

// ============================================================================
// PRIMITIVES
// ============================================================================

class Quad : public Hittable {
public:

    template <typename M>
    __host__ Quad(const M& material) 
        : Quad(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), material) { }

    template <typename M>
    __host__ Quad(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale, 
        const M&       material
    ) : Hittable(position, rotation, scale, material) { 
        Matrix3 R = xform.R().T();
        u = (R * Vector3(1.0f, 0.0f,  0.0f)) * xform.scale().x();
        v = (R * Vector3(0.0f, 0.0f, -1.0f)) * xform.scale().z();
        xform.set_position(xform.p() - (0.5f * u + 0.5f * v));
    }

    __device__ Quad(
        Transform& xform, 
        Transform& delta, 
        Material*  mat
    ) : Hittable(xform, delta, mat) { init(); }

    __host__ Hittable* build() const override {
        return device_build<Quad>(xform, delta, material);
    }

    __host__ const AABB build_bbox() const override {
        AABB box(
            AABB(xform.p(), xform.p() + u + v),
            AABB(xform.p() + u, xform.p() + v)
        );
        if (box.x.max - box.x.min < EPS) box.x = box.x.expand(EPS);
        if (box.y.max - box.y.min < EPS) box.y = box.y.expand(EPS);
        if (box.z.max - box.z.min < EPS) box.z = box.z.expand(EPS);
        return box;
    }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override {
        float denom = dot(normal, ray.direction());
        if (fabs(denom) < FMIN) return false;

        float t = (D - dot(normal, ray.origin())) / denom;
        if (!ray_t.contains(t)) return false;

        rec.t        = t;
        rec.position = ray.at(t);
        rec.material = material;
        rec.set_face_normal(ray, normal);

        return is_interior(rec);
    }

private:

    Vector3 u, v, w, normal;
    float D;

    __device__ void init() {
        Matrix3 R = xform.R().T();
        u = (R * Vector3(1.0f, 0.0f,  0.0f)) * xform.scale().x();
        v = (R * Vector3(0.0f, 0.0f, -1.0f)) * xform.scale().z();

        Vector3 n = cross(u, v);
        normal = normalize(n);
        w = n / dot(n, n);
        D = dot(normal, xform.p());
    }
    
    __device__ bool is_interior(HitRecord& rec) const {
        Interval unit_interval(0.0f, 1.0f);
        Vector3 planar_hitpt_vector = rec.position - xform.p();

        float a = dot(w, cross(planar_hitpt_vector, v));
        float b = dot(w, cross(u, planar_hitpt_vector));

        rec.uv = Vector2(a, b);
        return unit_interval.contains(a) & unit_interval.contains(b);
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
        Transform& xform, 
        Transform& delta,  
        float      rad, 
        Material*  mat
    ) : Hittable(xform, delta, mat), radius(rad) {}

    __host__ Hittable* build() const override {
        return device_build<Sphere>(xform, delta, radius, material);
    }

    __host__ const AABB build_bbox() const override {
        Vector3 rvec = Vector3(radius, radius, radius);
        return AABB(xform.p() - rvec, xform.p() + rvec);
    }

    __device__ const Vector2 get_uvs(const Vector3& p) const {
        float theta = acosf(-p.y());
        float phi   = atan2f(-p.z(), p.x()) + PI;
        return Vector2(phi / PI2, theta / PI);
    };

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
        rec.uv = get_uvs(norm);

        return true;
    }

private:

    float radius;
};

