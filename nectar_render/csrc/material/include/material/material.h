#pragma once

#include "hittable/include/hittable/hittable.h"

// ############################################################################
// ABSTRACT PARENT
// ############################################################################

class Material {
public:
    __host__ __device__ virtual ~Material() = default;
    __host__ virtual Material* build() const = 0;

    __device__ virtual bool scatter(
        const HitRecord& rec,
        Ray&       ray,
        Color&     attenuation,
        Generator& gen
    ) const { 
        return false; 
    }
};

// ############################################################################
// LAMBERTIAN
// ############################################################################

class Lambertian : public Material {
public:

    __host__ __device__ Lambertian(const Color& albedo) : albedo(albedo) {}

    __host__ Material* build() const override {
        return device_build<Lambertian>(albedo);
    }

    __device__ bool scatter(
        const HitRecord& rec,
        Ray&       ray,
        Color&     attenuation,
        Generator& gen
    ) const override { 
        Vector3 dir = rec.normal + random_unit_vector(gen);
        dir = dir.near_zero() ? rec.normal : dir;
        
        ray = Ray(rec.position, dir);
        attenuation *= albedo;
        return true;
    }

private:
    Color albedo;
};

// ############################################################################
// METAL
// ############################################################################



// ############################################################################
// DIELECTRIC
// ############################################################################



// ############################################################################
// EMISSIVE
// ############################################################################



