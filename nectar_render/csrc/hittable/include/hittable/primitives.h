#pragma once

#include "hittable/include/hittable/hittable.h"
#include "random/include/hash.h"

class Quad : public Hittable {
public:

    __host__ Quad() : Hittable(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f)) { }

    __host__ Quad(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale
    ) : Hittable(position, rotation, scale) { }

    __host__ Quad(const Material& material) 
        : Quad(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), material) { }

    template <typename M>
    __host__ Quad(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale, 
        const M&       material
    ) : Hittable(position, rotation, scale, material) { }

    __device__ Quad(
        Transform& xform, 
        Transform& delta, 
        size_t     material_index
    ) : Hittable(xform, delta, material_index) { 
        u = Vector3(1.0f, 0.0f, 0.0f);
        v = Vector3(0.0f, 0.0f, -1.0f);

        Vector3 n = cross(u, v);
        normal = normalize(n);
        w = n / dot(n, n);
        area = n.length();
    }

    __device__ Quad(
        Vector3& position,
        Vector3& rotation,
        Vector3& scale, 
        size_t   mat_idx
    ) { 
        xform = Transform(position, rotation, scale);
        material_index = mat_idx; 

        u = Vector3(1.0f, 0.0f, 0.0f);
        v = Vector3(0.0f, 0.0f, -1.0f);

        Vector3 n = cross(u, v);
        normal = normalize(n);
        w = n / dot(n, n);
        area = n.length();
    }

    __host__ Hittable* build() const override {
        return device_build<Quad>(xform, delta, material_index);
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

        rec.t = t;
        rec.p = ray.at(t);
        rec.n = normal;
        rec.tangent = u;

        return is_interior(rec);
    }

    __device__ float pdf_value(
        const Vector3& origin, 
        const Vector3& direction
    ) const override {
        HitRecord rec;
        Ray world_ray(origin, direction);
        if (!hit_test(world_ray, rec)) return 0.0f;

        float distance_squared = rec.t * rec.t * direction.length_squared();
        float cosine = fabsf(dot(direction, rec.n) / direction.length());
        if (cosine < 1e-8f) return 0.0f;

        float world_area = area * xform.scale().x() * xform.scale().z();
        return distance_squared / (cosine * world_area);
    }


    __device__ Vector3 random(
        const Vector3& origin,
        Generator& gen
    ) const override {
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

};

