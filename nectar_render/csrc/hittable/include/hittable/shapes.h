#pragma once

#include "primitives.h"
#include "core/include/core/onb.h"

class Sphere {
public:
    
    __host__ __device__ Sphere(float radius) : radius(radius + FMIN) { }

    __host__ Sphere* build() const {
        return device_build<Sphere>(radius);
    }

    __host__ const AABB build_bbox() const {
        return AABB(Vector3(-radius), Vector3(radius));
    }

    __device__ const Vector2 get_uvs(const Vector3& p) const {
        float theta = acosf(-p.y());
        float phi   = atan2f(-p.z(), p.x()) + PI;
        return Vector2(phi / PI2, theta / PI);
    };

    __device__ NOINLINE bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec,
        const Transform& xform
    ) const {
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

    __device__ float NOINLINE pdf_value(
        const Vector3& origin, 
        const Vector3& direction,
        const Transform& xform
    ) const {
        HitRecord rec;
        if (!this->hit(
            Ray(origin, direction), Interval(EPS, FMAX), rec, xform)
        ) return 0.0f;

        float rad_sq = radius * radius;
        float dist_squared = (xform.p() - origin).length_squared();
        float cos_theta_max = sqrtf(1.0f - rad_sq / dist_squared);
        float solid_angle = PI2 * (1.0f - cos_theta_max);

        return 1.0f / solid_angle;
    }

    __device__ Vector3 NOINLINE random(
        const Vector3& origin, 
        Generator& gen,
        const Transform& xform
    ) const {
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

class Cube {
public:

    __host__ Cube() { }

    __device__ Cube(Quad** faces) { 
        for (int i = 0; i < 6; i++) prims[i] = faces[i];
    }

    __host__ Cube* build() const {
        Quad* d_faces[6];

        d_faces[0] = device_build<Quad>(); // Front
        d_faces[1] = device_build<Quad>(); // Back
        d_faces[2] = device_build<Quad>(); // Right
        d_faces[3] = device_build<Quad>(); // Left
        d_faces[4] = device_build<Quad>(); // Top
        d_faces[5] = device_build<Quad>(); // Bottom

        Quad** d_face_ptrs;
        size_t n_bytes = 6 * sizeof(Quad*);
        cudaMalloc(&d_face_ptrs, n_bytes);
        cudaMemcpy(d_face_ptrs, d_faces, n_bytes, cudaMemcpyHostToDevice);
        return device_build<Cube>(d_face_ptrs);
    }

    __host__ const AABB build_bbox() const {
        return AABB(Vector3(-0.5f), Vector3(0.5f));
    }

    __device__ NOINLINE bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec,
        const Transform& xform
    ) const { 

        bool hit_anything = false;
        int  hit_face = -1;
        Ray  hit_face_ray;

        for (int i = 0; i < 6; i++) {
            Quad* current = prims[i];
            Transform curr_xform = face_xforms[i];
            Ray r = ray.to_object_space(curr_xform);

            HitRecord face_rec;
            if (current->hit(r, ray_t, face_rec, curr_xform)) {
                hit_anything = true;
                hit_face     = i;
                hit_face_ray = r;
                rec          = face_rec;
                ray_t.max    = face_rec.t;
            }
        }

        if (hit_anything) {
            rec.to_world_space(face_xforms[hit_face], hit_face_ray, ray);
        }

        return hit_anything;
    }

    __device__ float pdf_value(
        const Vector3& origin, 
        const Vector3& direction,
        const Transform& xform
    ) const { return 0.0f; }

    __device__ Vector3 random(
        const Vector3& origin, 
        Generator& gen,
        const Transform& xform
    ) const { return Vector3(1.0f, 0.0f, 0.0f); }


private:

    Quad* prims[6];

    Transform face_xforms[6] = {
        Transform( // Front
            Vector3(0.0f, 0.0f, 0.5f), 
            Vector3(90.0f, 0.0f, 0.0f), 
            Vector3(1.0f)
        ),
        Transform( // Back
            Vector3(0.0f, 0.0f, -0.5f), 
            Vector3(-90.0f, 0.0f, 0.0f), 
            Vector3(1.0f)
        ),
        Transform( // Right
            Vector3(0.5f, 0.0f, 0.0f), 
            Vector3(0.0f, 0.0f, -90.0f), 
            Vector3(1.0f)
        ),
        Transform( // Left
            Vector3(-0.5f, 0.0f, 0.0f), 
            Vector3(0.0f, 0.0f, 90.0f),
            Vector3(1.0f)
        ),
        Transform( // Top
            Vector3(0.0f, 0.5f, 0.0f), 
            Vector3(0.0f, 0.0f, 0.0f), 
            Vector3(1.0f)
        ),
        Transform( // Bottom
            Vector3(0.0f, -0.5f, 0.0f), 
            Vector3(180.0f, 0.0f, 0.0f), 
            Vector3(1.0f)
        )
    };

};

