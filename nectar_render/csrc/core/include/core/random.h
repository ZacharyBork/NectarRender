#pragma once

#include <random>
#include <curand.h>
#include <curand_kernel.h>

class Generator {
public:

    __host__ __device__ Generator(unsigned int seed)
        : seed_(seed), pixel_idx_(0), draw_id_(0) {}

    __host__ __device__ Generator(unsigned int seed, unsigned int pixel_idx)
        : seed_(seed), pixel_idx_(pixel_idx), draw_id_(0) {}

    __device__ float uniform() {
        curandStatePhilox4_32_10_t state;
        curand_init(seed_, pixel_idx_, draw_id_, &state);
        draw_id_ += 1;
        return curand_uniform(&state);
    }

    __device__ float normal() {
        curandStatePhilox4_32_10_t state;
        curand_init(seed_, pixel_idx_, draw_id_, &state);
        draw_id_ += 1;
        return curand_normal(&state);
    }

    __device__ float random_float(float min = 0.0f, float max = 1.0f) {
        return uniform() * (max - min) + min;
    }

    __device__ int random_int(int min = 0, int max = 10) {
        return (int)floorf(random_float((float)min, (float)max-1.0f));
    }

protected:
    unsigned int seed_;
    unsigned int pixel_idx_;
    unsigned int draw_id_;
    
};

__host__ inline float random_float(
    float min = 0.0f, 
    float max = 1.0f,
    unsigned int seed = 42
) {
    srand(seed);
    float value = static_cast<float>(rand()) / RAND_MAX;
    return value * (max - min) + min;
}

__host__ inline int random_int(
    int min = 0, 
    int max = 10,
    unsigned int seed = 42
) {
    srand(seed);
    return (int)floorf(random_float((float)min, (float)max-1.0f));
}
