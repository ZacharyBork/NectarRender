#pragma once

#include "core/include/core.h"
#include "hittable/include/hittable/hit_record.h"
#include "material/include/material/texture.h"
#include "engine/include/engine/pdf.h"

// ############################################################################
// UTILITIES
// ############################################################################

struct ScatterRecord {
    Color atten;
    PDF   pdf;
    bool  skip_pdf;
    Ray   skip_pdf_ray;
};

// ############################################################################
// ABSTRACT PARENT
// ############################################################################

class Material {
public:
    __host__ __device__ virtual ~Material() = default;
    __host__ virtual Material* build() const = 0;

    __device__ virtual void update(Material* mat) = 0;

    __device__ virtual bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
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
        : texture(ConstantTexture(albedo).build()) { }

    template<typename T>
    __host__ Lambertian(const T& texture) : texture(texture.build()) {}

    __device__ Lambertian(Texture* texture) : texture(texture) { }

    __host__ Material* build() const override {
        return device_build<Lambertian>(texture);
    }

    __device__ virtual void update(Material* mat) override {
        Lambertian* other = reinterpret_cast<Lambertian*>(mat);
        texture = other->texture;
    };

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const override { 
        srec.atten = texture->sample(rec.uv, rec.p);
        srec.pdf   = PDF::cosine(rec.n);
        srec.skip_pdf = false;
        return true;
    }

    __device__ float scattering_pdf(
        const HitRecord& rec,
        const Ray& r_in,
        const Ray& r_scattered
    ) const override { 
        float cos_theta = dot(rec.n, normalize(r_scattered.direction()));
        return cos_theta < 0.0f ? 0.0f : cos_theta / PI;
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

    __device__ virtual void update(Material* mat) override {
        Metal* other = reinterpret_cast<Metal*>(mat);
        albedo = other->albedo;
        fuzz = other->fuzz;
    };

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const override { 
        Vector3 reflected = reflect(ray.direction(), rec.n);
        reflected = normalize(reflected) + (fuzz * random_unit_vector(gen));
        
        srec.atten        = albedo;
        srec.skip_pdf     = true;
        srec.skip_pdf_ray = Ray(rec.p, reflected, ray.time());

        return true;
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

    __device__ virtual void update(Material* mat) override {
        Dielectric* other = reinterpret_cast<Dielectric*>(mat);
        ior = other->ior;
    };

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const override { 
        srec.atten    = Color::white();
        srec.skip_pdf = true;

        float ri = rec.front_face ? (1.0f / ior) : ior;

        Vector3 unit_direction = normalize(ray.direction());
        float cos_theta = fminf(dot(-unit_direction, rec.n), 1.0f);
        float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        Vector3 direction;

        if (cannot_refract || reflectance(cos_theta, ri) > gen.uniform())
            direction = reflect(unit_direction, rec.n);
        else direction = refract(unit_direction, rec.n, ri);

        srec.skip_pdf_ray = Ray(rec.p, direction, ray.time());
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
        return r0 + (1.0f - r0) * powf((1.0f - cosine), 5.0f);
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

    __device__ virtual void update(Material* mat) override {
        Emissive* other = reinterpret_cast<Emissive*>(mat);
        texture = other->texture;
        brightness = other->brightness;
    };

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

    __device__ virtual void update(Material* mat) override {
        Isotropic* other = reinterpret_cast<Isotropic*>(mat);
        texture = other->texture;
    };

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const override { 
        srec.atten = texture->sample(rec.uv, rec.p);
        srec.pdf   = PDF::sphere();
        srec.skip_pdf = false;
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

// ############################################################################
// PBR MATERIAL
// ############################################################################

struct PBRDescription {
    Texture* albedo = nullptr;

        
    Texture* roughness = nullptr;

    Texture* metallic = nullptr;
    
    Texture* normal;
    float normal_strength = 1.0f;

    Texture* ambient_occlusion = nullptr;
    float ao_power = 1.0f;


};

class PBRMaterial : public Material {
public:

    template<typename T>
    __host__ PBRMaterial(
        const T& albedo,
        const T& roughness,
        const T& metallic,
        const T& normal,
        float normal_strength,
        const T& ambient_occlusion,
        float ao_power
    ) : desc({ 
            albedo.build(),
            roughness.build(),
            metallic.build(),
            normal.build(),
            normal_strength,
            ambient_occlusion.build(),
            ao_power
        })
    { }

    __device__ PBRMaterial(PBRDescription description) 
        : desc(description) { }

    __host__ Material* build() const override {
        return device_build<PBRMaterial>(desc);
    }

    __device__ virtual void update(Material* mat) override { };

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const override { 
        Color normal_map = desc.normal->sample(rec.uv, rec.p);
        rec.n = normalize(
            rec.n + normal_map.as_vector().pow(desc.normal_strength)
        );

        srec.atten = desc.albedo->sample(rec.uv, rec.p);
        srec.pdf   = PDF::phong(rec.n, 1.0f);
        srec.skip_pdf = false;

        return true;
    }

    __device__ float scattering_pdf(
        const HitRecord& rec,
        const Ray& r_in,
        const Ray& r_scattered
    ) const override { 
        float cos_theta = dot(rec.n, normalize(r_scattered.direction()));
        return cos_theta < 0.0f ? 0.0f : cos_theta / PI;
    }

private:

    PBRDescription desc;

};
