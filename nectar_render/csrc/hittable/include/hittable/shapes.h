#pragma once

#include "core/include/core/onb.h"
#include "hittable/include/hittable/hittable.h"
#include "hittable/include/hittable/primitives.h"

class Sphere : public Hittable {
public:
    
    __host__ Sphere(
        const Vector3& position, 
        float radius,
        const Material& material
    ) : Hittable(position, material), 
        radius(radius + FMIN) 
    { }

    __device__ Sphere(
        Transform& xform, 
        Transform& delta,  
        float      rad, 
        size_t     material_index
    ) : Hittable(xform, delta, material_index), radius(rad) {}

    __host__ Hittable* build() const override {
        return device_build<Sphere>(xform, delta, radius, material_index);
    }

    __host__ const AABB build_bbox() const override {
        return AABB::simple(Vector3(radius), xform);
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

        rec.t = root;
        rec.p = ray.at(rec.t);
        
        Vector3 norm = rec.p / (radius * xform.scale() + FMIN);
        rec.n = norm;
        rec.tangent = normalize(Vector3(-rec.p.z(), 0.0f, rec.p.x()));
        rec.uv = get_uvs(norm);

        return true;
    }

    __device__ float pdf_value(
        const Vector3& origin, 
        const Vector3& direction
    ) const override {
        HitRecord rec;
        if (!this->hit(Ray(origin, direction), Interval(EPS, FMAX), rec))
            return 0.0f;

        float rad_sq = radius * radius;
        float dist_squared = (xform.p() - origin).length_squared();
        float cos_theta_max = sqrtf(1.0f - rad_sq / dist_squared);
        float solid_angle = PI2 * (1.0f - cos_theta_max);

        return 1.0f / solid_angle;
    }

    __device__ Vector3 random(
        const Vector3& origin, 
        Generator& gen
    ) const override {
        Vector3 direction = xform.p() - origin;
        auto distance_squared = direction.length_squared();
        ONB uvw(direction);
        return uvw.transform(random_to_sphere(radius, distance_squared, gen));
    }

private:

    float radius;

    __device__ static Vector3 random_to_sphere(
        float radius, 
        float distance_squared,
        Generator& gen
    ) {
        float r1 = gen.random_float();
        float r2 = gen.random_float();
        float rad_sq = radius * radius;
        float z = 1.0f + r2 * (sqrtf(1.0f - rad_sq / distance_squared) - 1.0f);

        float phi = PI2 * r1;
        float x = cosf(phi) * sqrtf(1.0f - z * z);
        float y = sinf(phi) * sqrtf(1.0f - z * z);

        return Vector3(x, y, z);
    }

};

class Cube : public Hittable {
public:
    
    __host__ Cube(const Material& material) 
        : Cube(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), material) { }

    __host__ Cube(
        const Vector3&  position,
        const Vector3&  rotation,
        const Vector3&  scale, 
        const Material& material
    ) : Hittable(position, rotation, scale, material) { }

    __device__ Cube(
        Transform& xform, 
        Transform& delta, 
        Hittable** faces,
        size_t     material_index
    ) : Hittable(xform, delta, material_index) { 
        for (int i = 0; i < 6; i++) prims[i] = faces[i];
    }

    __host__ Hittable* build() const override {
        Hittable* d_faces[6];

        d_faces[0] = Quad( // Front
            Vector3(0.0f, 0.0f, 0.5f), 
            Vector3(90.0f, 0.0f, 0.0f), 
            Vector3(1.0f),
            material
        ).build();
        d_faces[1] = Quad( // Back
            Vector3(0.0f, 0.0f, -0.5f), 
            Vector3(-90.0f, 0.0f, 0.0f), 
            Vector3(1.0f),
            material
        ).build();
        d_faces[2] = Quad( // Right
            Vector3(0.5f, 0.0f, 0.0f), 
            Vector3(0.0f, 0.0f, -90.0f), 
            Vector3(1.0f),
            material
        ).build();
        d_faces[3] = Quad( // Left
            Vector3(-0.5f, 0.0f, 0.0f), 
            Vector3(0.0f, 0.0f, 90.0f),
            Vector3(1.0f),
            material
        ).build();
        d_faces[4] = Quad( // Top
            Vector3(0.0f, 0.5f, 0.0f), 
            Vector3(0.0f, 0.0f, 0.0f), 
            Vector3(1.0f),
            material
        ).build();
        d_faces[5] = Quad( // Bottom
            Vector3(0.0f, -0.5f, 0.0f), 
            Vector3(180.0f, 0.0f, 0.0f), 
            Vector3(1.0f),
            material
        ).build();

        Hittable** d_face_ptrs;
        cudaMalloc(&d_face_ptrs, 6 * sizeof(Hittable*));
        cudaMemcpy(
            d_face_ptrs, d_faces, 
            6 * sizeof(Hittable*), 
            cudaMemcpyHostToDevice
        );

        return device_build<Cube>(xform, delta, d_face_ptrs, material_index);
    }

    __host__ const AABB build_bbox() const override {
        return AABB::oriented(Vector3(0.5f), xform).buffer();
    }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override { 

        bool hit_anything = false;
        int  hit_face = -1;
        Ray  hit_face_ray;

        for (int i = 0; i < 6; i++) {
            Hittable* current = prims[i];
            Ray r = ray.to_object_space(current->xform);

            HitRecord face_rec;
            if (current->hit(r, ray_t, face_rec)) {
                hit_anything = true;
                hit_face     = i;
                hit_face_ray = r;
                rec          = face_rec;
                ray_t.max    = face_rec.t;
            }
        }

        if (hit_anything) {
            rec.to_world_space(prims[hit_face]->xform, hit_face_ray, ray);
        }

        return hit_anything;
    }


private:

    Hittable* prims[6];
};

