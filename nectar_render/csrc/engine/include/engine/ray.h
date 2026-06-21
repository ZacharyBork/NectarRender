#pragma once

#include "core/include/core/vector.h"

class Ray {
public:
    Ray() { }

    __host__ __device__ Ray(
        const Point3& origin, const Vector3& direction
    ) : orig(origin), dir(direction) { }

    __host__ __device__ const Point3& origin() const { return orig; }
    __host__ __device__ const Vector3& direction() const { return dir; }

    __host__ __device__ Point3 at(float t) const { return orig + t * dir; }

private:
    Point3  orig;
    Vector3 dir;

};

