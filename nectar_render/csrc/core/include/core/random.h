#pragma once

#include <random>
#include <curand.h>
#include <curand_kernel.h>

class Generator {
public:
    __device__ Generator(unsigned int seed, unsigned int pixel_idx)
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

private:
    unsigned int seed_;
    unsigned int pixel_idx_;
    unsigned int draw_id_;
};

