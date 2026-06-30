#pragma once

#include "hittable/include/hittable/hittable.h"
#include "hittable/include/hittable/primitives.h"

class Sphere : public Hittable {
public:
    
    template <typename M>
    __host__ Sphere(const Vector3& position, float radius, const M& material) 
        : Hittable(position, material), 
          radius(radius + FMIN) 
    { }

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
        Vector3 rad = radius * xform.scale();
        return AABB(xform.position() - rad, xform.position() + rad);
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
        Vector3 oc = -ray.origin();

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

        rec.t   = root;
        rec.p   = ray.at(rec.t);
        rec.mat = material;
        
        Vector3 norm = rec.p / (radius * xform.scale() + FMIN);
        rec.n = norm;
        rec.uv = get_uvs(norm);

        return true;
    }

private:

    float radius;
};

// class Cube : public Hittable {
// public:
    
//     template <typename M>
//     __host__ Cube(const M& material) 
//         : Cube(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), material) { }

//     template <typename M>
//     __host__ Cube(
//         const Vector3& position,
//         const Vector3& rotation,
//         const Vector3& scale, 
//         const M&       material
//     ) : Hittable(position, rotation, scale, material) { 
//         Matrix3 R = xform.R().T();
//         u = (R * Vector3(1.0f, 0.0f,  0.0f)) * xform.scale().x();
//         v = (R * Vector3(0.0f, 0.0f, -1.0f)) * xform.scale().z();
//         xform.set_position(xform.p() - (0.5f * u + 0.5f * v));
//     }

//     __device__ Cube(
//         Transform& xform, 
//         Transform& delta, 
//         Material*  mat
//     ) : Hittable(xform, delta, mat) { init(); }

//     __host__ Hittable* build() const override {
//         return device_build<Cube>(xform, delta, material);
//     }

//     __host__ const AABB build_bbox() const override {
//         AABB box(
//             Vector3(-0.5f, -0.5f, -0.5f),
//             Vector3( 0.5f,  0.5f,  0.5f)
//         );
//         return box;
//     }

//     __device__ bool hit(
//         const Ray& ray, 
//         Interval   ray_t,
//         HitRecord& rec
//     ) const override {
//         float denom = dot(normal, ray.direction());
//         if (fabs(denom) < FMIN) return false;

//         float t = (D - dot(normal, ray.origin())) / denom;
//         if (!ray_t.contains(t)) return false;

//         rec.t        = t;
//         rec.position = ray.at(t);
//         rec.material = material;
//         rec.set_face_normal(ray, normal);

//         return is_interior(rec);
//     }

// private:

//     Quad* prims[6];

// };

