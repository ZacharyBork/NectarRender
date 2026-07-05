#pragma once

#include "core/include/core.h"
#include "engine/include/engine/transform.h"

class Ray {
public:

    __host__ __device__ Ray() 
        : Ray(Vector3(0.0f), Vector3(0.0f, 0.0f, 1.0f), 0.0f)
    { }    

    __host__ __device__ Ray(
        const Vector3& origin, const Vector3& direction, float time = 0.0f
    ) : orig(origin), dir(direction), tm(time) { }

    __host__ __device__ float time()               const { return tm; }
    __host__ __device__ const Vector3& origin()    const { return orig; }
    __host__ __device__ const Vector3& direction() const { return dir; }

    __host__ __device__ const Vector3& rO() const { return orig; }
    __host__ __device__ const Vector3& rD() const { return dir; }

    __host__ __device__ Vector3 at(float t) const { return orig + t * dir; }

    __device__ Ray to_object_space(const Transform& xform) const {
        Vector3 origin = (xform.inv_R() * (orig-xform.pos())) * xform.inv_s();
        Vector3 direction = (xform.inv_R() * dir) * xform.inv_s();
        return Ray(origin, direction, tm);
    }

    __device__ Ray clone() { return Ray(orig, dir, tm); }

private:

    Vector3 orig;
    Vector3 dir;
    float   tm;

};

