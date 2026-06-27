#pragma once

#include <array>

#include "core/include/core.h"

struct Transform {
    Vector3 position_, scale_;
    Matrix3 rotation_;

    __host__ __device__ Transform() 
      : position_(Vector3(0.0f, 0.0f, 0.0f)),
        rotation_(rotation_from_euler(deg2rad(Vector3(0.0f, 0.0f, 0.0f)))),
        scale_(Vector3(1.0f, 1.0f, 1.0f)) 
    { }

    __host__ __device__ Transform(const Vector3& p) : position_(p) { }
    
    __host__ __device__ Transform(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale
    ) : position_(position), 
        rotation_(rotation_from_euler(deg2rad(rotation))), 
        scale_(scale) 
    { }

    __host__ __device__ Transform(
        std::array<float, 3> position,
        std::array<float, 3> rotation,
        std::array<float, 3> scale
    ) : Transform(Vector3(position), Vector3(rotation), Vector3(scale)) { }

    __host__ __device__ const Vector3& position() const { return position_; }
    __host__ __device__ const Matrix3& rotation() const { return rotation_; }
    __host__ __device__ const Vector3& scale()    const { return scale_;    }

    __host__ __device__ void set_position(const Vector3& p) { position_ = p; }
    __host__ __device__ void set_rotation(const Matrix3& r) { rotation_ = r; }
    __host__ __device__ void set_scale   (const Vector3& s) { scale_    = s; }

    __host__ __device__ const Vector3& p()   const { return position(); }
    __host__ __device__ const Vector3& pos() const { return position(); }
    __host__ __device__ const Matrix3& R()   const { return rotation(); }
};


