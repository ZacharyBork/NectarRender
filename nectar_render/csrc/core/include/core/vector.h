#pragma once

#include <cuda_runtime.h>
#include <iostream>
#include "core/include/core/random.h"

//=============================================================================
// VECTOR2
//=============================================================================

class Vector2 {
public:
    float e[2];

    __host__ __device__ Vector2() : e{ 0, 0 } {}
    __host__ __device__ Vector2(float e0, float e1) : e{ e0, e1 } {}

    __host__ __device__ float x() const { return e[0]; }
    __host__ __device__ float y() const { return e[1]; }
    __host__ __device__ float u() const { return e[0]; }
    __host__ __device__ float v() const { return e[1]; }

    __host__ __device__ Vector2 operator-() const { 
        return Vector2(-e[0], -e[1]); 
    }
    __host__ __device__ float   operator[](int i) const { return e[i]; }
    __host__ __device__ float&  operator[](int i) { return e[i]; }

    __host__ __device__ Vector2& operator+=(const Vector2& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        return *this;
    }

    __host__ __device__ Vector2& operator*=(float t) {
        e[0] *= t;
        e[1] *= t;
        return *this;
    }

    __host__ __device__ Vector2& operator/=(float t) {
        return *this *= 1/t;
    }

    __host__ __device__ float length() const {
        return std::sqrt(length_squared());
    }

    __host__ __device__ float length_squared() const {
        return e[0]*e[0] + e[1]*e[1];
    }
};

using Vec2   = Vector2;
using Point2 = Vector2;

//=============================================================================
// VECTOR3
//=============================================================================

class Vector3 {
public:
    float e[3];

    __host__ __device__ Vector3() : e{ 0, 0, 0 } {}
    __host__ __device__ Vector3(
        float e0, float e1, float e2
    ) : e{ e0, e1, e2 } {}

    __host__ __device__ float x() const { return e[0]; }
    __host__ __device__ float y() const { return e[1]; }
    __host__ __device__ float z() const { return e[2]; }

    __host__ __device__ float u() const { return e[0]; }
    __host__ __device__ float v() const { return e[1]; }
    __host__ __device__ float w() const { return e[2]; }

    __host__ __device__ float r() const { return e[0]; }
    __host__ __device__ float g() const { return e[1]; }
    __host__ __device__ float b() const { return e[2]; }

    __host__ __device__ Vector3 operator-() const { 
        return Vector3(-e[0], -e[1], -e[2]); 
    }
    __host__ __device__ float   operator[](int i) const { return e[i]; }
    __host__ __device__ float&  operator[](int i) { return e[i]; }

    __host__ __device__ Vector3& operator+=(const Vector3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    __host__ __device__ Vector3& operator*=(float t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    __host__ __device__ Vector3& operator/=(float t) {
        return *this *= 1/t;
    }

    __host__ __device__ float length() const {
        return std::sqrt(length_squared());
    }

    __host__ __device__ float length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    static __device__ Vector3 random(Generator& gen) {
        return Vector3(
            gen.random_in_range(),
            gen.random_in_range(), 
            gen.random_in_range()
        );
    }

    static __device__ Vector3 random(
        float      min, 
        float      max,
        Generator& gen
    ) {
        return Vector3(
            gen.random_in_range(min, max), 
            gen.random_in_range(min, max), 
            gen.random_in_range(min, max)
        );
    }
};

/* ALIASING */

using Vec3   = Vector3;
using Point3 = Vector3;
using Color  = Vector3;

/* UTILITIES */

// INSPECTION

// inline __host__ __device__ std::ostream& operator<<(
//     std::ostream& out, const Vector3& v
// ) {
//     return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
// }

// DEFAULT OPERATORS

inline __host__ __device__ Vector3 operator+(
    const Vector3& u, const Vector3& v
) {
    return Vector3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}
inline __host__ __device__ Vector3 operator+(const Vector3& u, float t) {
    return Vector3(u.e[0] + t, u.e[1] + t, u.e[2] + t);
}
inline __host__ __device__ Vector3 operator+(float t, const Vector3& u) {
    return u + t;
}


inline __host__ __device__ Vector3 operator-(
    const Vector3& u, const Vector3& v
) {
    return Vector3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}
inline __host__ __device__ Vector3 operator-(const Vector3& u, float t) {
    return Vector3(u.e[0] - t, u.e[1] - t, u.e[2] - t);
}
inline __host__ __device__ Vector3 operator-(float t, const Vector3& u) {
    return u - t;
}


inline __host__ __device__ Vector3 operator*(
    const Vector3& u, const Vector3& v
) {
    return Vector3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}
inline __host__ __device__ Vector3 operator*(const Vector3& u, float t) {
    return Vector3(u.e[0] * t, u.e[1] * t, u.e[2] * t);
}
inline __host__ __device__ Vector3 operator*(float t, const Vector3& u) {
    return u * t;
}


inline __host__ __device__ Vector3 operator/(
    const Vector3& u, const Vector3& v
) {
    return Vector3(u.e[0] / v.e[0], u.e[1] / v.e[1], u.e[2] / v.e[2]); 
}
inline __host__ __device__ Vector3 operator/(const Vector3& u, float t) {
    return (1 / t) * u;
}
inline __host__ __device__ Vector3 operator/(float t, const Vector3& u) {
    return Vector3(t / u.e[0], t / u.e[1], t / u.e[2]);
}

// MATH

inline __host__ __device__ float dot(const Vector3& u, const Vector3& v) {
    return u.e[0] * v.e[0]
         + u.e[1] * v.e[1]
         + u.e[2] * v.e[2];
}

inline __host__ __device__ Vector3 cross(const Vector3& u, const Vector3& v) {
    return Vector3(
        u.e[1] * v.e[2] - u.e[2] * v.e[1],
        u.e[2] * v.e[0] - u.e[0] * v.e[2],
        u.e[0] * v.e[1] - u.e[1] * v.e[0]
    );
}

// CREATION

inline __device__ Vector3 unit_vector(const Vector3& v) {
    return v / v.length();
}

inline __device__ Vector3 random_unit_vector(Generator& gen) {
    while (true) {
        auto p = Vector3::random(-1.0f, 1.0f, gen);
        auto lensq = p.length_squared();
        if (lensq <= 1)
            return p / sqrt(lensq);
    }
}
