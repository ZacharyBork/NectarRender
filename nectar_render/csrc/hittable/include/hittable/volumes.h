#pragma once

#include "random/include/hash.h"

class Hittable;

class ConstantMedium {
public:

    __host__ __device__ ConstantMedium(float density) 
        : neg_inv_density(-1.0f / (density + FMIN)) { }

    template<typename T>
    __device__ NOINLINE  bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec,
        T*  wrapped_object
    ) const {
        HitRecord rec1, rec2;

        if (!wrapped_object->hit(ray, Interval(-FMAX, FMAX), rec1))
            return false;

        if (!wrapped_object->hit(ray, Interval(rec1.t + EPS, FMAX), rec2))
            return false;

        if (rec1.t < ray_t.min) rec1.t = ray_t.min;
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;
        if (rec1.t >= rec2.t)   return false;
        if (rec1.t < 0.0f)      rec1.t = 0.0f;

        auto ray_length = ray.direction().length();
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
        auto hit_distance = neg_inv_density * logf(pcg_float(ray));

        if (hit_distance > distance_inside_boundary)
            return false;

        rec.p = ray.at(rec1.t + hit_distance / ray_length);
        rec.n = Vector3(1.0f, 0.0f, 0.0f);
        rec.front_face = true;

        return true;
    }

private:

    float neg_inv_density;

};

