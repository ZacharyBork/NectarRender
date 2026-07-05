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

    __device__ virtual float scattering_pdf(
        const HitRecord& rec,
        const Ray& r_in,
        const Ray& r_scattered
    ) const { 
        return 0.0f; 
    }

    __device__ virtual Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const {
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
        ONB uvw(rec.n);

        Vector3 dir = uvw.transform(random_cosine_direction(gen));
        dir = dir.near_zero() ? rec.n : dir;
        
        ray = Ray(rec.p, dir, ray.time());
        attenuation *= texture->sample(rec.uv, rec.p);
        return true;
    }

    __device__ float scattering_pdf(
        const HitRecord& rec,
        const Ray& r_in,
        const Ray& r_scattered
    ) const override { 
        float cosine = dot(rec.n, normalize(r_scattered.direction()));
        return fmaxf(0.0f, cosine / PI);
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

    __host__ Emissive(const Color& albedo, const float brightness = 35.0f) 
        : texture(ConstantTexture(albedo).build()), brightness(brightness) {}

    template<typename T>
    __host__ Emissive(const T& texture, const float brightness = 35.0f) 
        : texture(texture.build()), brightness(brightness) {}

    __device__ Emissive(Texture* texture, float brightness) 
        : texture(texture), brightness(brightness) { }

    __host__ Material* build() const override {
        return device_build<Emissive>(texture, brightness);
    }

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const override {
        if (!rec.front_face) return Color::black();
        return texture->sample(rec.uv, rec.p) * brightness;
    }

private:

    Texture* texture;
    float    brightness;

};

// ############################################################################
// EMISSIVE
// ############################################################################

class Isotropic : public Material {
public:

    __host__ Isotropic(const Color& albedo) 
        : texture(ConstantTexture(albedo).build()) {}

    template<typename T>
    __host__ Isotropic(const T& texture) : texture(texture.build()) {}

    __device__ Isotropic(Texture* texture) : texture(texture) { }

    __host__ Material* build() const override {
        return device_build<Isotropic>(texture);
    }

    __device__ bool scatter(
        const HitRecord& rec,
        Ray&       ray,
        Color&     attenuation,
        Generator& gen
    ) const override { 
        ray = Ray(rec.p, random_unit_vector(gen), ray.time());
        attenuation *= texture->sample(rec.uv, rec.p);
        return true;
    }

    __device__ float scattering_pdf(
        const HitRecord& rec,
        const Ray& r_in,
        const Ray& r_scattered
    ) const override { 
        return 1.0f / PI4;
    }

private:

    Texture* texture;
};


