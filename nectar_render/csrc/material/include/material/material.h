#pragma once

#include "core/include/core.h"
#include "hittable/include/hittable/hit_record.h"
#include "material/include/material/texture.h"

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

    __device__ virtual Color emitted(Vector2 uv, const Vector3& p) const {
        return Color::black();
    }
};

// ############################################################################
// LAMBERTIAN
// ############################################################################

class Lambertian : public Material {
public:

    __host__ Lambertian(const Color& albedo) 
        : texture(ConstantTexture(albedo).build()) {}

    template<typename T>
    __host__ Lambertian(const T& texture) : texture(texture.build()) {}

    __device__ Lambertian(Texture* texture) : texture(texture) { }

    __host__ Material* build() const override {
        return device_build<Lambertian>(texture);
    }

    __device__ bool scatter(
        const HitRecord& rec,
        Ray&       ray,
        Color&     attenuation,
        Generator& gen
    ) const override { 
        Vector3 dir = rec.n + random_unit_vector(gen);
        dir = dir.near_zero() ? rec.n : dir;
        
        ray = Ray(rec.p, dir, ray.time());
        attenuation *= texture->sample(rec.uv, rec.p);
        return true;
    }

private:

    Texture* texture;
};

// ############################################################################
// METAL
// ############################################################################

class Metal : public Material {
public:

    __host__ __device__ Metal(const Color& albedo, float fuzz = 0.0f) 
        : albedo(albedo), fuzz(fuzz) {}

    __host__ Material* build() const override {
        return device_build<Metal>(albedo, fuzz);
    }

    __device__ bool scatter(
        const HitRecord& rec,
        Ray&       ray,
        Color&     attenuation,
        Generator& gen
    ) const override { 
        Vector3 reflected = reflect(ray.direction(), rec.n);
        reflected = normalize(reflected) + (fuzz * random_unit_vector(gen));
        ray = Ray(rec.p, reflected, ray.time());
        attenuation *= albedo;
        return (dot(ray.direction(), rec.n) > 0);
    }

private:
    Color albedo;
    float fuzz;
};

// ############################################################################
// DIELECTRIC
// ############################################################################

class Dielectric : public Material {
public:

    __host__ __device__ Dielectric(float ior = 1.5f) : ior(ior) {}

    __host__ Material* build() const override {
        return device_build<Dielectric>(ior);
    }

    __device__ bool scatter(
        const HitRecord& rec,
        Ray&       ray,
        Color&     attenuation,
        Generator& gen
    ) const override { 
        float ri = rec.front_face ? (1.0f / ior) : ior;

        Vector3 unit_direction = normalize(ray.direction());
        float cos_theta = fminf(dot(-unit_direction, rec.n), 1.0f);
        float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        Vector3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > gen.uniform())
            direction = reflect(unit_direction, rec.n);
        else direction = refract(unit_direction, rec.n, ri);

        ray = Ray(rec.p, direction, ray.time());
        attenuation *= Color(1.0f, 1.0f, 1.0f);
        return true;
    }

private:
    float ior;

    __device__ static float reflectance(
        float cosine, 
        float refraction_index
    ) {
        float r0 = (1.0f - refraction_index) / (1.0f + refraction_index);
        r0 = r0 * r0;
        return r0 + (1-r0) * powf((1.0f - cosine), 5.0f);
    }
};

// ############################################################################
// EMISSIVE
// ############################################################################

class Emissive : public Material {
public:

    __host__ Emissive(const Color& albedo) 
        : texture(ConstantTexture(albedo).build()) {}

    template<typename T>
    __host__ Emissive(const T& texture) : texture(texture.build()) {}

    __device__ Emissive(Texture* texture) : texture(texture) { }

    __host__ Material* build() const override {
        return device_build<Emissive>(texture);
    }

    __device__ Color emitted(Vector2 uv, const Vector3& p) const override {
        return texture->sample(uv, p);
    }

private:

    Texture* texture;
};

