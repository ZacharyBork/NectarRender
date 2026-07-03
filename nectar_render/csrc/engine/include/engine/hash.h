// ############################################################################
// HASHING LIBRARY
// 
// Defines hash functions for in-place pseudo-random number generation on host 
// and device.
// ############################################################################

#pragma once
#include "core/include/core.h"
#include "engine/include/engine/ray.h"

// ############################################################################
// HASHING UTILITIES
// ############################################################################

__host__ __device__ inline constexpr float HASH_SCALAR = 1.f/(float)UINT32_MAX;
__host__ __device__ __constant__ inline uint32_t HASH_CONSTANTS[64] = {
    0x9E3779B9u, 0x85EBCA6Bu, 0xC2B2AE35u, 0x27D4EB2Fu, 
    0x165667B1u, 0x68D2A5B5u, 0xBF58476Du, 0x94D049BBu, 
    0xA3B195B5u, 0x1B873593u, 0xCC9E2D51u, 0x6C62272Eu, 
    0xE654025Bu, 0x2B5AD5B5u, 0x6C696E65u, 0x736F6D65u, 
    0x646F6D65u, 0x70736575u, 0x6E646F6Du, 0xFF51AFD7u,
    0xC4CEA4D6u, 0x45D9F3Bu,  0x3335B369u, 0xA341316Cu, 
    0x52784632u, 0x68656C6Cu, 0x6F20776Fu, 0x726C6421u, 
    0xB5297A4Du, 0x68E31DA4u, 0x1B56C4E9u, 0xD2A98B26u, 
    0x637B2B53u, 0x4F6E4F35u, 0x62DC5AB7u, 0x5A17A2C3u, 
    0x6EF305DAu, 0x2C9F410Du, 0x3771A6A7u, 0x15C3A7C4u,  
    0x8F43CD0Bu, 0xA9DE8FA3u, 0xEBF7A401u, 0x58399E71u, 
    0x42B91A0Du, 0x4B608671u, 0xB41B21C7u, 0x2D5A73B7u,
    0xE50B4C1Fu, 0x95C48A21u, 0x37A07BA9u, 0xC0F4A537u,
    0xA8FA763Bu, 0xD08BE5EDu, 0x4A9C5EB3u, 0x7B14EEA7u, 
    0xCC3A6547u, 0x9B89D66Du, 0xE15E5E3Bu, 0x4F3D8A5Fu,  
    0xD4E0A97Bu, 0x3C6EF372u, 0xA54FF53Au, 0x510E527Fu,  
};

__host__ __device__ inline uint32_t hash_lookup(uint32_t idx) {
    return HASH_CONSTANTS[idx & 63u];
}

__host__ __device__ inline uint32_t __hash_mix(
    uint32_t val, 
    uint32_t constant_idx
) {
    val ^= (val >> 16u) * hash_lookup(constant_idx);
    return val^(val >> 13u);
}

__host__ __device__ inline uint32_t __pcg_hash_int(
    const uint32_t i,
    const uint32_t seed   = 0u,
    const uint32_t stream = 0u
) {
    uint32_t s = (seed * 1759274928u) ^ (stream * 2246822519u);
    s = i ^ s;
    s = __hash_mix(s, stream * 3u);
    s = __hash_mix(s, stream * 3u + 1u);
    s = __hash_mix(s, stream * 3u + 2u);

    uint32_t state = s * 747796405u + 2891336453u;
    uint32_t word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

__host__ __device__ inline uint32_t __pcg_hash_fp(
    const float    f,
    const uint32_t seed   = 0u,
    const uint32_t stream = 0u
) {
    return __pcg_hash_int(float_as_uint(f), seed, stream);
}

// ############################################################################
// PCG HASHING (FLOAT)
// ############################################################################

__host__ __device__ inline float pcg_float(
    const float    f,
    const uint32_t seed   = 0u,
    const uint32_t stream = 0u
) {
    uint32_t hash = __pcg_hash_fp(f, seed, stream);
    return (float)hash * HASH_SCALAR;
}

__host__ __device__ inline float pcg_float(
    const Vector3& a,
    const uint32_t seed = 0u,
    const uint32_t stream = 0u
) {
    uint32_t s = seed * 1759274928u;
    s ^= __pcg_hash_fp(a.x(), (seed + 0u) * 2654435761u, stream) ^ 668265263u;
    s ^= __pcg_hash_fp(a.y(), (seed + 1u) * 2246822519u, stream) ^ 374761393u;
    s ^= __pcg_hash_fp(a.z(), (seed + 2u) * 3266489917u, stream) ^ 2654435761u;

    uint32_t state = s * 747796405u + 2891336453u;
    uint32_t word  = ((state >> ((state >> 28u) + 4u))^state) * 277803737u;
    return (float)((word >> 22u) ^ word) * HASH_SCALAR;
}

__host__ __device__ inline float pcg_float(
    const Vector3& a,
    const Vector3& b,
    const uint32_t seed = 0u,
    const uint32_t stream = 0u
) {
    float a_hash = pcg_float(a, (seed + 0u) * 2654435761u, stream);
    float b_hash = pcg_float(b, (seed + 1u) * 3266489917u, stream);
    return (a_hash + b_hash) * 0.5f;
}

__host__ __device__ inline float pcg_float(
    const Ray& ray, 
    const uint32_t seed = 0u,
    const uint32_t stream = 0u
) {
    return pcg_float(ray.origin(), ray.direction(), seed, stream);
}

// ############################################################################
// DEVICE-ONLY
// ############################################################################

__device__ inline float device_pcg_float(
    const uint32_t seed = 0u,
    const uint32_t stream = 0u
) {
    ProcessIndex p_idx = get_process_index();
    return pcg_float(Vector3(p_idx.x, p_idx.y, p_idx.z), seed, stream);
}

__device__ inline uint32_t device_pcg_int(
    const uint32_t min    = 0u,
    const uint32_t max    = 99u,
    const uint32_t seed   = 0u,
    const uint32_t stream = 0u
) {
    ProcessIndex p_idx = get_process_index();
    uint32_t hash = seed * 1759274928u;
    hash += __pcg_hash_int(p_idx.x, seed + 0u, stream);
    hash += __pcg_hash_int(p_idx.y, seed + 1u, stream);
    hash += __pcg_hash_int(p_idx.z, seed + 2u, stream);
    return (hash % (max - min)) + min;
}

__device__ inline Vector3 device_pcg_vector3(
    const uint32_t seed = 0u,
    const uint32_t stream = 0u
) {
    ProcessIndex p_idx = get_process_index();
    return Vector3(
        pcg_float((float)p_idx.x, seed + 0u, stream),
        pcg_float((float)p_idx.y, seed + 1u, stream),
        pcg_float((float)p_idx.z, seed + 2u, stream)
    );
}

__device__ inline Color device_pcg_color(
    const uint32_t seed = 0u,
    const uint32_t stream = 0u
) {
    ProcessIndex p_idx = get_process_index();
    return Color(
        pcg_float((float)p_idx.x, seed + 0u, stream),
        pcg_float((float)p_idx.y, seed + 1u, stream),
        pcg_float((float)p_idx.z, seed + 2u, stream)
    );
}

