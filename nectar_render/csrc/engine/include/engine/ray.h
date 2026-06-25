#pragma once

#include "core/include/core/vector.h"

class Ray {
public:
    __host__ __device__ Ray() { 
        Ray(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), 0.0f);
    }

    __host__ __device__ Ray(
        const Vector3& origin, const Vector3& direction, float time = 0.0f
    ) : orig(origin), dir(direction), tm(time) { }

    __host__ __device__ float time()               const { return tm; }
    __host__ __device__ const Vector3&  origin()   const { return orig; }
    __host__ __device__ const Vector3& direction() const { return dir; }

    __host__ __device__ Vector3 at(float t) const { return orig + t * dir; }

private:

    Vector3 orig;
    Vector3 dir;
    float   tm;

};

