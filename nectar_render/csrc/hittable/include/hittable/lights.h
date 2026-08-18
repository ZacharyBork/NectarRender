#pragma once

#include "engine/include/engine/ray.h"

class ObjectLight {
public:

    __host__ __device__ ObjectLight() { }

    template<typename T>
    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec,
        T* wrapped_object
    ) const {
        if (!wrapped_object->hit(ray, Interval(-FMAX, FMAX), rec))
            return false;
        return true;
    }

    template<typename T>
    __device__ float pdf_value(
        const Vector3& origin, 
        const Vector3& direction,
        T* wrapped_object
    ) const { return wrapped_object->pdf_value(origin, direction); }

    template<typename T>
    __device__ Vector3 random(
        const Vector3& origin,
        Generator& gen,
        T* wrapped_object
    ) const { return wrapped_object->random(origin, gen); }

};




