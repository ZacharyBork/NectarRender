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

class Cube : public Hittable {
public:
    
    template <typename M>
    __host__ Cube(const M& material) 
        : Cube(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), material) { }

    template <typename M>
    __host__ Cube(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale, 
        const M&       material
    ) : Hittable(position, rotation, scale, material) { }

    __device__ Cube(
        Transform& xform, 
        Transform& delta, 
        Material*  mat,
        Hittable** faces
    ) : Hittable(xform, delta, mat) { 
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

        return device_build<Cube>(xform, delta, material, d_face_ptrs);
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

