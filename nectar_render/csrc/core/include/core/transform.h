#pragma once

#include <array>

#include "core/include/core/constants.h"
#include "core/include/core/vector.h"
#include "core/include/core/matrix.h"
#include "core/include/core/utils.h"

struct Transform {
public:
    __host__ __device__ Transform() 
      : position_(Vector3(0.0f)),
        rotation_(rotation_from_euler(Vector3(0.0f))),
        scale_(Vector3(1.0f)),
        inv_rotation_(rotation_from_euler(Vector3(0.0f)).T()),
        inv_scale_(Vector3(1.0f))
    { }

    __host__ __device__ Transform(const Vector3& p) 
        : position_(p),
          rotation_(rotation_from_euler(Vector3(0.0f))),
          scale_(Vector3(1.0f)),
          inv_rotation_(rotation_from_euler(Vector3(0.0f)).T()),
          inv_scale_(Vector3(1.0f))
    { }
    
    __host__ __device__ Transform(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale
    ) : position_(position), 
        rotation_(rotation_from_euler(deg2rad(rotation))), 
        scale_(scale),
        inv_rotation_(rotation_from_euler(deg2rad(rotation)).T()),
        inv_scale_(Vector3(1.0f) / (scale + FMIN))
    { }

    __host__ __device__ Transform(
        const Vector3& position,
        const Matrix3& rotation,
        const Vector3& scale
    ) : position_(position), 
        rotation_(rotation), 
        scale_(scale),
        inv_rotation_(rotation.T()),
        inv_scale_(Vector3(1.0f) / (scale + FMIN))
    { }

    __host__ __device__ const Matrix3& inv_R() const { return inv_rotation_; }
    __host__ __device__ const Vector3& inv_s() const { return inv_scale_;    }

    __host__ __device__ const Vector3& position() const { return position_; }
    __host__ __device__ const Matrix3& rotation() const { return rotation_; }
    __host__ __device__ const Vector3& scale()    const { return scale_;    }

    __host__ __device__ void set_position(const Vector3& p) { position_ = p; }
    __host__ __device__ void set_scale(const Vector3& s) { 
        scale_ = s; 
        inv_scale_ = Vector3(1.0f) / (s + FMIN);
    }
    __host__ __device__ void set_rotation(const Matrix3& r) { 
        rotation_ = r; 
        inv_rotation_ = r.T();
    }

    __host__ __device__ const Vector3& p()   const { return position(); }
    __host__ __device__ const Vector3& pos() const { return position(); }
    __host__ __device__ const Matrix3& R()   const { return rotation(); }

    __device__ const Transform current() const {
        return Transform(position(), rotation(), scale());
    }

    
private:
    Vector3 position_, scale_;
    Matrix3 rotation_;
    Vector3 inv_scale_;
    Matrix3 inv_rotation_;
};

