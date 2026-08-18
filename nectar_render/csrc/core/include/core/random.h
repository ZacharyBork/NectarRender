#pragma once

#include <random>
#include <curand.h>
#include <curand_kernel.h>

// ############################################################################
// DEVICE-SIDE RANDOM NUMBER GENERATOR
// ############################################################################

class Generator {
public:

    /* CONSTRUCTORS */

    __host__ __device__ Generator() 
        : seed_(0u), pixel_idx_(0u), draw_id_(0u) {}

    __host__ __device__ Generator(uint32_t seed)
        : seed_(seed), pixel_idx_(0u), draw_id_(0u) {}

    __device__ Generator(uint32_t seed, uint32_t pixel_idx)
        : seed_(seed), pixel_idx_(pixel_idx), draw_id_(0u) {}

    /* UTILITIES */

    __host__ __device__ uint32_t get_seed() const { 
        return seed_; 
    }
    __host__ __device__ void set_seed(uint32_t seed) { 
        seed_ = seed; 
    }

    __host__ __device__ uint32_t get_pixel_idx() const { 
        return pixel_idx_; 
    }
    __host__ __device__ void set_pixel_idx(uint32_t idx) { 
        pixel_idx_ = idx; 
    }

    __host__ __device__ uint32_t get_draw_id() const { 
        return draw_id_; 
    }
    __host__ __device__ void set_draw_id(uint32_t id) { 
        draw_id_ = id; 
    }

    /* GENERATORS */

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
    uint32_t seed_;
    uint32_t pixel_idx_;
    uint32_t draw_id_;
    
};



// ############################################################################
// HOST-SIDE RNG UTILITIES
// ############################################################################

__host__ inline float random_float(
    float min = 0.0f, 
    float max = 1.0f,
    uint32_t seed = 42
) {
    srand(seed);
    float value = static_cast<float>(rand()) / RAND_MAX;
    return value * (max - min) + min;
}

__host__ inline int random_int(
    int min = 0, 
    int max = 10,
    uint32_t seed = 42
) {
    srand(seed);
    return (int)floorf(random_float((float)min, (float)max-1.0f));
}
