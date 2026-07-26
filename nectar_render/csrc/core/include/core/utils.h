#pragma once

#include <stdint.h>
#include <cuda_runtime.h>

#include "core/include/core/vector.h"

// ############################################################################
// VALUE CONVERSION
// ############################################################################

__host__ __device__ inline float deg2rad(float degrees) {
    return degrees * 0.01745329; 
}

__host__ __device__ inline Vector3 deg2rad(const Vector3& degrees) {
    return Vector3(
        deg2rad(degrees.x()), 
        deg2rad(degrees.y()), 
        deg2rad(degrees.z())
    );
}

__host__ __device__ inline float rad2deg(float radians) {
    return radians * 57.29578;
}

__host__ __device__ inline Vector3 rad2deg(const Vector3& radians) {
    return Vector3(
        rad2deg(radians.x()), 
        rad2deg(radians.y()), 
        rad2deg(radians.z())
    );
}

// ############################################################################
// VALUE INTERPRETATION
// ############################################################################

__host__ __device__ inline uint32_t float_as_uint(float f) {
    uint32_t u;
    memcpy(&u, &f, sizeof(u));
    return u;
}

// ############################################################################
// ACCESS GUARDING
// ############################################################################

template<typename T>
class Guarded {
public:

    __host__ Guarded(const char* label = "resource") 
        : ptr_(nullptr), label_(label) {}
    
    __host__ explicit Guarded(T* ptr, const char* label = "resource") 
        : ptr_(ptr), label_(label) {}

    __host__ void enable(T* ptr)     { ptr_ = ptr; }
    __host__ void disable()          { ptr_ = nullptr; }
    __host__ bool is_enabled() const { return ptr_ != nullptr; }

    __host__ T* operator->() const {
        if (!ptr_)
            throw std::runtime_error(
                std::string("Invalid access to disabled/uninitialized ") 
                + label_
            );
        return ptr_;
    }

    __host__ T* pointer() { return ptr_;}

private:

    T*          ptr_;
    const char* label_;

};



