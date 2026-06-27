#pragma once

#include <array>
#include <vector>
#include <iostream>
#include <type_traits>
#include <cuda_runtime.h>

#include "core/include/core/random.h"
#include "core/include/core/constants.h"

//=============================================================================
// ABSTRACT PARENT
//=============================================================================

template <typename T>
constexpr std::size_t component_count = std::extent_v<decltype(T::e)>;

template<typename VecType>
class Vector {
public:

    __host__ __device__ float   operator[](int i) const { 
        auto& self = derived();
        return self.e[i]; 
    }
    __host__ __device__ float&  operator[](int i) { 
        auto& self = derived();
        return self.e[i]; 
    }

    __host__ __device__ VecType operator-() const { 
        VecType result = derived();
        #pragma unroll
        for (size_t i = 0; i < component_count<VecType>; ++i)
            result.e[i] = -result.e[i];
        return result;
    }

    __host__ __device__ VecType& operator+=(const VecType& v) {
        auto& self = derived();
        #pragma unroll
        for (size_t i = 0; i < component_count<VecType>; ++i)
            self.e[i] += v.e[i];
        return self;
    }

    __host__ __device__ VecType operator-=(const VecType& v) { 
        auto& self = derived();
        #pragma unroll
        for (size_t i = 0; i < component_count<VecType>; ++i)
            self.e[i] -= v.e[i];
        return self;
    }

    __host__ __device__ VecType& operator*=(const VecType& v) {
        auto& self = derived();
        #pragma unroll
        for (size_t i = 0; i < component_count<VecType>; ++i)
            self.e[i] *= v.e[i];
        return self;
    }

    __host__ __device__ VecType& operator/=(float t) {
        return *this *= 1.0f / (t + FMAX);
    }

    __host__ __device__ float length() const {
        return sqrtf(length_squared());
    }

    __host__ __device__ float length_squared() const {
        auto& self = derived();
        float out = self.e[0] * self.e[0] + self.e[1] * self.e[1];
        if constexpr (component_count<VecType> >= 3) 
            out += self.e[2] * self.e[2];
        if constexpr (component_count<VecType> >= 4) 
            out += self.e[3] * self.e[3];
        return out;
    }

    __host__ __device__ bool near_zero(float eps = 1e-8) const {
        return derived().length() < eps;
    }

    __host__ __device__ VecType& exp() const {
        VecType result = derived();
        #pragma unroll
        for (size_t i = 0; i < component_count<VecType>; ++i)
            result.e[i] = expf(result.e[i]);
        return result;
    }

private:

    __host__ __device__ VecType& derived() { 
        return static_cast<VecType&>(*this); 
    }

    __host__ __device__ const VecType& derived() const { 
        return static_cast<const VecType&>(*this); 
    }
};

template<typename T>
using is_vector = std::enable_if_t<std::is_base_of_v<Vector<T>, T>>;

//=============================================================================
// OPERATORS
//=============================================================================

template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator+(const T& u, const T& v) {
    T result = u;
    result += v;
    return result;
}
template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator+(const T& u, float t) {
    T result = u;
    #pragma unroll
    for (size_t i = 0; i < component_count<T>; ++i)
        result.e[i] += t;
    return result;
}
template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator+(float t, const T& u) { return u + t; }


template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator-(const T& u, const T& v) {
    T result = u;
    result -= v;
    return result;
}
template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator-(const T& u, float t) {
    T result = u;
    #pragma unroll
    for (size_t i = 0; i < component_count<T>; ++i)
        result.e[i] -= t;
    return result;
}
template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator-(float t, const T& u) { return u - t; }


template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator*(const T& u, const T& v) {
    T result = u;
    result *= v;
    return result;
}
template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator*(const T& u, float t) {
    T result = u;
    #pragma unroll
    for (size_t i = 0; i < component_count<T>; ++i)
        result.e[i] *= t;
    return result;
}
template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator*(float t, const T& u) { return u * t; }


template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator/(const T& u, const T& v) {
    T result = u;
    result /= v;
    return result;
}
template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator/(const T& u, float t) {
    return (1.0f / t) * u;
}
template <typename T, typename = is_vector<T>>
__host__ __device__ inline T operator/(float t, const T& u) {
    T result = u;
    #pragma unroll
    for (size_t i = 0; i < component_count<T>; ++i)
        result.e[i] = t / u.e[i];
    return result;
}

//=============================================================================
// UTILITIES
//=============================================================================

template <typename T, typename = is_vector<T>>
__host__ __device__ inline T normalize(const T& v) { return v / v.length(); }

//=============================================================================
// VECTOR2
//=============================================================================

template<typename VecType>
class Vec2Core : public Vector<VecType> {
public:
    float e[2];

    __host__ __device__ Vec2Core() : e{ 0, 0 } {}
    __host__ __device__ Vec2Core(float e) : e{ e, e } {}
    __host__ __device__ Vec2Core(float e0, float e1) : e{ e0, e1 } {}
    
    __host__ Vec2Core(std::array<float, 2> e) : e{e[0], e[1]} {}
    __host__ Vec2Core(float e[4]) : e{e[0], e[1]} {}

    template <typename OtherType>
    __host__ __device__ explicit Vec2Core(const Vec2Core<OtherType>& other)
        : e{other.e[0], other.e[1]} {}

    __host__ __device__ float x() const { return e[0]; }
    __host__ __device__ float y() const { return e[1]; }
    __host__ __device__ float u() const { return e[0]; }
    __host__ __device__ float v() const { return e[1]; }

    __device__ static VecType sample_square(Generator& gen) {
        return VecType(gen.random_float()-0.5, gen.random_float()-0.5);
    }

    __device__ static VecType random_in_unit_disk(Generator& gen) {
        while (true) {
            VecType p = VecType(
                gen.random_float(-1.0f, 1.0f), 
                gen.random_float(-1.0f, 1.0f)
            );
            if (p.length_squared() < 1.0f) return p;
        }
    }
};

class Vector2 : public Vec2Core<Vector2> { using Vec2Core::Vec2Core; };
class Point2  : public Vec2Core<Point2>  { using Vec2Core::Vec2Core; };

//=============================================================================
// VECTOR3
//=============================================================================

template<typename VecType>
class Vec3Core : public Vector<VecType> {
public:
    float e[3];

    __host__ __device__ Vec3Core() : e{ 0, 0, 0 } {}
    __host__ __device__ Vec3Core(float e) : e{ e, e, e } {}
    __host__ __device__ Vec3Core(
        float e0, float e1, float e2
    ) : e{ e0, e1, e2 } {}

    __host__ Vec3Core(std::array<float, 3> e) : e{e[0], e[1], e[2]} {}
    __host__ Vec3Core(float e[4]) : e{e[0], e[1], e[2]} {}
    

    template <typename OtherType>
    __host__ __device__ explicit Vec3Core(const Vec3Core<OtherType>& other)
        : e{other.e[0], other.e[1], other.e[2]} {}

    __host__ __device__ float x() const { return e[0]; }
    __host__ __device__ float y() const { return e[1]; }
    __host__ __device__ float z() const { return e[2]; }

    __host__ __device__ float u() const { return e[0]; }
    __host__ __device__ float v() const { return e[1]; }
    __host__ __device__ float w() const { return e[2]; }

    __device__ static VecType random(Generator& gen) {
        return VecType(
            gen.random_float(),
            gen.random_float(), 
            gen.random_float()
        );
    }

    __device__ static VecType random(
        float      min, 
        float      max,
        Generator& gen
    ) {
        return VecType(
            gen.random_float(min, max), 
            gen.random_float(min, max), 
            gen.random_float(min, max)
        );
    }
};

class Vector3 : public Vec3Core<Vector3> { using Vec3Core::Vec3Core; };
class Color   : public Vec3Core<Color>   { using Vec3Core::Vec3Core; 
public:
    __host__ __device__ float r() const { return e[0]; }
    __host__ __device__ float g() const { return e[1]; }
    __host__ __device__ float b() const { return e[2]; }

    __host__ __device__ static Color black()  {return Color(0.0f, 0.0f, 0.0f);}
    __host__ __device__ static Color white()  {return Color(1.0f, 1.0f, 1.0f);}
    __host__ __device__ static Color red()    {return Color(1.0f, 0.0f, 0.0f);}
    __host__ __device__ static Color green()  {return Color(0.0f, 1.0f, 0.0f);}
    __host__ __device__ static Color blue()   {return Color(0.0f, 0.0f, 1.0f);}
    __host__ __device__ static Color purple() {return Color(1.0f, 0.0f, 1.0f);}
    __host__ __device__ static Color yellow() {return Color(1.0f, 1.0f, 0.0f);}
    __host__ __device__ static Color teal()   {return Color(0.0f, 1.0f, 1.0f);}
};

// MATH =======================================================================

__host__ __device__ inline float dot(const Vector3& u, const Vector3& v) {
    return u.e[0] * v.e[0]
         + u.e[1] * v.e[1]
         + u.e[2] * v.e[2];
}

__host__ __device__ inline Vector3 cross(const Vector3& u, const Vector3& v) {
    return Vector3(
        u.e[1] * v.e[2] - u.e[2] * v.e[1],
        u.e[2] * v.e[0] - u.e[0] * v.e[2],
        u.e[0] * v.e[1] - u.e[1] * v.e[0]
    );
}

__host__ __device__ inline Vector3 reflect(
    const Vector3& v,
    const Vector3& n
) { 
    return v - 2.0f * dot(v, n) * n; 
}

__host__ __device__ inline Vector3 refract(
    const Vector3& uv,
    const Vector3& n,
    float ior
) { 
    float cos_theta = fminf(dot(-uv, n), 1.0f);
    Vector3 perp =  ior * (uv + cos_theta * n);
    Vector3 parallel = -sqrtf(fabsf(1.0 - perp.length_squared())) * n;
    return perp + parallel;
}

// CREATION ===================================================================

__device__ inline Vector3 random_unit_vector(Generator& gen) {
    while (true) {
        auto p = Vector3::random(-1.0f, 1.0f, gen);
        auto lensq = p.length_squared();
        if (1e-160 < lensq && lensq <= 1)
            return p / sqrt(lensq);
    }
}

__device__ inline Vector3 random_on_hemisphere(
    const Vector3& normal,
    Generator& gen
) {
    Vector3 on_unit_sphere = random_unit_vector(gen);
    if (dot(on_unit_sphere, normal) > 0.0f)
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}
