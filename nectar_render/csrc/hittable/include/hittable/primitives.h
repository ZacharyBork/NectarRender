#pragma once

#include "random/include/hash.h"

class Quad {
public:

    __host__ __device__ Quad() { 
        u = Vector3(1.0f, 0.0f, 0.0f);
        v = Vector3(0.0f, 0.0f, -1.0f);

        Vector3 n = cross(u, v);
        normal = normalize(n);
        w = n / dot(n, n);
        area = n.length();
    }

    __host__ Quad* build() const { return device_build<Quad>(); }

    __host__ const AABB build_bbox() const {
        return AABB(Vector3(-0.5f), Vector3(0.5f));
    }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec,
        const Transform& xform
    ) const {
        float denom = dot(normal, ray.direction());
        if (fabs(denom) < FMIN) return false;

        float t = -dot(normal, ray.origin()) / denom;
        if (!ray_t.contains(t)) return false;

        rec.t = t;
        rec.p = ray.at(t);
        rec.n = normal;
        rec.tangent = u;

        return is_interior(rec);
    }

    __device__ NOINLINE float pdf_value(
        const Vector3&   origin, 
        const Vector3&   direction,
        const Transform& xform
    ) const {
        HitRecord rec;
        Ray world_ray(origin, direction);
        if (!hit_test(world_ray, rec, xform)) return 0.0f;

        float distance_squared = rec.t * rec.t * direction.length_squared();
        float cosine = fabsf(dot(direction, rec.n) / direction.length());
        if (cosine < 1e-8f) return 0.0f;

        float world_area = area * xform.scale().x() * xform.scale().z();
        return distance_squared / (cosine * world_area);
    }


    __device__ NOINLINE Vector3 random(
        const Vector3& origin,
        Generator& gen,
        const Transform& xform
    ) const {
        Vector3 corner = -0.5f * u - 0.5f * v;
        Vector3 p = corner + (gen.random_float()*u) + (gen.random_float()*v);
        p = xform.R() * (p * xform.scale()) + xform.p();
        return p - origin;
    }

private:

    Vector3 u, v, w, normal;
    float area;

    __device__ bool is_interior(HitRecord& rec) const {
        Interval unit_interval(-0.5f, 0.5f);
        Vector3 planar_hitpt_vector = rec.p;

        float a = dot(w, cross(planar_hitpt_vector, v));
        float b = dot(w, cross(u, planar_hitpt_vector));

        rec.uv = Vector2(a + 0.5f, b + 0.5f);
        return unit_interval.surrounds(a) 
             & unit_interval.surrounds(b);
    }

    __device__ bool hit_test(
        const Ray& ray, 
        HitRecord& rec,
        const Transform& xform
    ) const {
        Ray r = ray.to_object_space(xform);
        bool hit_obj = hit(r, Interval(EPS, FMAX), rec, xform);
        rec.to_world_space(xform, r, ray, true);
        return hit_obj;
    }

};

