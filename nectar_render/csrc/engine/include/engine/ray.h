#pragma once

#include "core/include/core/vector.h"

class Ray {
public:
    __host__ __device__ Ray() { 
        Ray(Point3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), 0.0f);
    }

    __host__ __device__ Ray(
        const Point3& origin, const Vector3& direction, float time = 0.0f
    ) : orig(origin), dir(direction), t(time) { }

    __host__ __device__ const Point3&  origin()    const { return orig; }
    __host__ __device__ const Vector3& direction() const { return dir; }
    __host__ __device__ const float&   time()      const { return t; }

    __host__ __device__ Point3 at(float t) const { return orig + t * dir; }

private:
    Point3  orig;
    Vector3 dir;
    float   t;

};

