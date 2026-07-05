#pragma once

#include "core/include/core/vector.h"

class ONB {
public:

    __device__ ONB(const Vector3& n) {
        axis[2] = normalize(n);
        Vector3 a = (fabsf(axis[2].x()) > 0.9f) ? 
            Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
        axis[1] = normalize(cross(axis[2], a));
        axis[0] = cross(axis[2], axis[1]);
    }

    __device__ const Vector3& u() const { return axis[0]; }
    __device__ const Vector3& v() const { return axis[1]; }
    __device__ const Vector3& w() const { return axis[2]; }

    __device__ Vector3 transform(const Vector3& v) const {
        return (v[0] * axis[0]) + (v[1] * axis[1]) + (v[2] * axis[2]);
    }

private:

    Vector3 axis[3];

};
