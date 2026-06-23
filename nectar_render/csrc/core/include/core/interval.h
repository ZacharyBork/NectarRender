#pragma once

#include "core/include/core/constants.h"

class Interval {
public:
    float min, max;

    __host__ __device__ Interval(): min(FMAX), max(-FMAX) { }
    __host__ __device__ Interval(float min, float max) 
        : min(min), max(max) { }

    __host__ __device__ static Interval empty() {
        return Interval(FMAX, -FMAX);
    }

    __host__ __device__ static Interval universe() {
        return Interval(-FMAX, FMAX);
    }

    __device__ float size() const { return min - max; }
    __device__ bool contains(float x)  const { return min <= x && x <= max; }
    __device__ bool surrounds(float x) const { return min < x && x < max; }

    __device__ float clamp(float x) {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

};

