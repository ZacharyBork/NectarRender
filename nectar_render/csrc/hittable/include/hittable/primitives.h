#pragma once

#include "hittable/include/hittable/hittable.h"

class Quad : public Hittable {
public:

    __host__ Quad() : Hittable(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f)) { }

    __host__ Quad(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale
    ) : Hittable(position, rotation, scale) { }

    template <typename M>
    __host__ Quad(const M& material) 
        : Quad(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), material) { }

    template <typename M>
    __host__ Quad(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale, 
        const M&       material
    ) : Hittable(position, rotation, scale, material) { }

    __host__ Quad(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale, 
        Material*      material
    ) : Hittable(position, rotation, scale, material) { }

    __device__ Quad(
        Transform& xform, 
        Transform& delta, 
        Material*  mat
    ) : Hittable(xform, delta, mat) { 
        u = Vector3(1.0f, 0.0f, 0.0f);
        v = Vector3(0.0f, 0.0f, -1.0f);

        Vector3 n = cross(u, v);
        normal = normalize(n);
        w = n / dot(n, n);
    }

    __host__ Hittable* build() const override {
        return device_build<Quad>(xform, delta, material);
    }

    __host__ const AABB build_bbox() const override {
        return AABB::oriented(Vector3(0.5f), xform).buffer();
    }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override {
        float denom = dot(normal, ray.direction());
        if (fabs(denom) < FMIN) return false;

        float t = -dot(normal, ray.origin()) / denom;
        if (!ray_t.contains(t)) return false;

        rec.t   = t;
        rec.p   = ray.at(t);
        rec.mat = material;
        rec.n   = normal;

        return is_interior(rec);
    }

private:

    Vector3 u, v, w, normal;

    __device__ bool is_interior(HitRecord& rec) const {
        Interval unit_interval(-0.5f, 0.5f);
        Vector3 planar_hitpt_vector = rec.p;

        float a = dot(w, cross(planar_hitpt_vector, v));
        float b = dot(w, cross(u, planar_hitpt_vector));

        rec.uv = Vector2(a + 0.5f, b + 0.5f);
        return unit_interval.surrounds(a) 
             & unit_interval.surrounds(b);
    }

};

