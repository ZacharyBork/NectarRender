/**
 * Re-implements useful functions from Nvidia's CG Standard Library, primarily 
 * to add overloads for NectarRender vector classes.
 * 
 * Original library reference found here:
 *     https://developer.nvidia.com/w/cg/index_stdlib.html
 */

#pragma once

#include "vector.h"

namespace CG {

    // MIN ====================================================================

    __host__ __device__ inline float min(const float x, const float y) {
        return fminf(x, y);
    }

    template<typename VecType>
    __host__ __device__ inline VecType min(
        const VecType& x, const VecType& y
    ) {
        return x.minimum(y);
    }

    template<typename VecType>
    __host__ __device__ inline VecType min(
        const VecType& x, const float y
    ) {
        return x.minimum(y);
    }

    template<typename VecType>
    __host__ __device__ inline VecType min(
        const float x, const VecType& y
    ) {
        return CG::min(y, x);
    }

    // MAX ====================================================================

    __host__ __device__ inline float max(const float x, const float y) {
        return fmaxf(x, y);
    }

    template<typename VecType>
    __host__ __device__ inline VecType max(
        const VecType& x, const VecType& y
    ) {
        return x.maximum(y);
    }

    template<typename VecType>
    __host__ __device__ inline VecType max(
        const VecType& x, const float y
    ) {
        return x.maximum(y);
    }

    template<typename VecType>
    __host__ __device__ inline VecType max(
        const float x, const VecType& y
    ) {
        return CG::max(y, x);
    }

    // CLAMP ==================================================================

    template<typename T>
    __host__ __device__ inline T clamp(const T& x, T& lo, T hi) {
        return CG::min(hi, CG::max(lo, x));
    }

    template<typename T>
    __host__ __device__ inline T clamp(const T& x, float lo, float hi) {
        return CG::min(hi, CG::max(lo, x));
    }

    // SATURATE ===============================================================

    template<typename T>
    __host__ __device__ inline T saturate(const T& x) {
        return CG::max(0.0f, CG::min(1.0f, x));
    }

    // LERP ===================================================================

    template<typename T>
    __host__ __device__ inline T lerp(const T& x, const T& y, float weight) {
        return x + weight * (y - x);
    }

    // SMOOTHSTEP =============================================================

    template<typename T>
    __host__ __device__ inline T smoothstep(
        const T& a, const T& b, const T& x
    ) {
        float t = saturate((x - a) / (b - a));
        return t * t * (3.0f - (2.0f * t));
    }

}
