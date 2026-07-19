#pragma once

#include <assert.h>
#include <cuda_runtime.h>

#include "core/include/core.h"
#include "material/include/material/texture.h"
#include "engine/include/engine/pdf.h"

// ############################################################################
// UTILITIES
// ############################################################################

class HitRecord;
class MaterialCore {}; // Dummy parent for common type-erased pointer.

struct ScatterRecord {
    Color atten;
    PDF   pdf;
    bool  skip_pdf;
    Ray   skip_pdf_ray;
};

// ############################################################################
// LAMBERTIAN
// ############################################################################

class Lambertian : public MaterialCore {
public:

    __host__ Lambertian(const Color& albedo) 
        : texture(ConstantTexture(albedo).build()) { }

    template<typename T>
    __host__ Lambertian(const T& texture) : texture(texture.build()) {}

    __device__ Lambertian(Texture* texture) : texture(texture) { }

    __device__ void update(MaterialCore* mat) {
        Lambertian* other = reinterpret_cast<Lambertian*>(mat);
        texture = other->texture;
    };

    __host__ void teardown() { if (texture) cudaFree(texture); }

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const { 
        srec.atten = texture->sample(rec.uv, rec.p);
        srec.pdf   = PDF::cosine(rec.n);
        srec.skip_pdf = false;
        return true;
    }

    __device__ Color evaluate(
        const HitRecord& rec, 
        const Vector3& view_dir, 
        const Vector3& light_dir
    ) const {
        float cos_theta = dot(rec.n, normalize(light_dir));
        if (cos_theta <= 0.0f) return Color::black();
        return texture->sample(rec.uv, rec.p) * (cos_theta / PI);
    }

private:

    Texture* texture;
    
};

// ############################################################################
// PBR METAL ROUGHNESS
// ############################################################################

class PBR : public MaterialCore {
public:

    template<typename T>
    __host__ PBR(
        const T& albedo,
        const T& roughness,
        const T& metallic,
        const T& emission,
        const T& normal
    ) : albedo_tex   (albedo.build()), 
        roughness_tex(roughness.build()), 
        metallic_tex (metallic.build()),
        emission_tex (emission.build()), 
        normal_tex   (normal.build())
    { }

    __device__ PBR(
        Texture* albedo,
        Texture* roughness,
        Texture* metallic,
        Texture* emission,
        Texture* normal
    ) : albedo_tex   (albedo), 
        roughness_tex(roughness), 
        metallic_tex (metallic),
        emission_tex (emission), 
        normal_tex   (normal)
    { }

    __device__ void update(MaterialCore* mat) {
        PBR* other = reinterpret_cast<PBR*>(mat);
        albedo_tex    = other->albedo_tex;
        roughness_tex = other->roughness_tex;
        metallic_tex  = other->metallic_tex;
        emission_tex  = other->emission_tex;
        normal_tex    = other->normal_tex;
    }

    __host__ void teardown() { 
        if (albedo_tex)    cudaFree(albedo_tex);
        if (roughness_tex) cudaFree(roughness_tex);
        if (metallic_tex)  cudaFree(metallic_tex);
        if (emission_tex)  cudaFree(emission_tex);
        if (normal_tex)    cudaFree(normal_tex);
    }

    __device__ NOINLINE Vector3 get_shading_normal(const HitRecord& rec) const {
        Color tex_n = normal_tex->sample(rec.uv, rec.p);
        Vector3 map_n = normalize(Vector3(
            tex_n.r() * 2.0f - 1.0f, 
            tex_n.g() * 2.0f - 1.0f, 
            tex_n.b() * 2.0f - 1.0f
        ));
        Vector3 t = normalize(rec.tangent - rec.n * dot(rec.tangent, rec.n));
        Vector3 b = cross(rec.n, t);
        return normalize(t * map_n.x() + b * map_n.y() + rec.n * map_n.z());
    }

    __device__ NOINLINE bool scatter(
        HitRecord& rec, 
        Ray& ray, 
        ScatterRecord& srec, 
        Generator& gen
    ) const {
        Vector3 n        = get_shading_normal(rec);
        Color   albedo   = albedo_tex->sample(rec.uv, rec.p);
        float   metallic = metallic_tex->sample(rec.uv, rec.p).r();

        Color F0 = Color(0.04f) * (1.0f - metallic) + albedo * metallic;
        float spec_prob = fminf(
            0.9f, fmaxf(0.1f, fmaxf(F0.r(), fmaxf(F0.g(), F0.b())))
        );

        srec.atten    = Color::white();
        srec.skip_pdf = false;
        srec.pdf      = PDF::pbr_lobe(
            n, normalize(-ray.direction()), 
            roughness_tex->sample(rec.uv, rec.p).r(), 
            spec_prob
        );
        return true;
    }

    __device__ NOINLINE Color evaluate(
        const HitRecord& rec, 
        const Vector3& view_dir, 
        const Vector3& light_dir
    ) const {
        Vector3 n = get_shading_normal(rec);
        float n_dot_l = dot(n, light_dir);
        float n_dot_v = dot(n, view_dir);
        if (n_dot_l <= 0.0f || n_dot_v <= 0.0f) return Color::black();

        Color albedo    = albedo_tex->sample(rec.uv, rec.p);
        float roughness = roughness_tex->sample(rec.uv, rec.p).r();
        float metallic  = metallic_tex->sample(rec.uv, rec.p).r();
        float alpha     = fmaxf(roughness * roughness, 1e-3f);

        Vector3 h = normalize(view_dir + light_dir);
        float n_dot_h = fmaxf(dot(n, h), 0.0f);
        float v_dot_h = fmaxf(dot(view_dir, h), 0.0f);

        Color F0 = Color(0.04f) * (1.0f - metallic) + albedo * metallic;
        Color F  = F0 + (Color::white() - F0) * powf(1.0f - v_dot_h, 5.0f);
        float D  = ggx_distribution(n_dot_h, alpha);
        float G  = smith_ggx_geometry(n_dot_l, n_dot_v, alpha);

        Color specular = F * (D * G / fmaxf(4.0f * n_dot_v * n_dot_l, 1e-4f));
        Color diffuse  = albedo * (1.0f - metallic) * (1.0f / PI);

        return (diffuse + specular) * n_dot_l;
    }

    __device__ NOINLINE Color emitted(
        const Ray& ray, 
        const HitRecord& rec
    ) const {
        if (!rec.front_face) return Color::black();
        return emission_tex->sample(rec.uv, rec.p);
    }

private:

    Texture* albedo_tex;
    Texture* roughness_tex;
    Texture* metallic_tex;
    Texture* emission_tex;
    Texture* normal_tex;

};

// ############################################################################
// DIELECTRIC
// ############################################################################

class Dielectric : public MaterialCore {
public:

    __host__ __device__ Dielectric(float ior = 1.5f) : ior(ior) {}

    __device__ void update(MaterialCore* mat) {
        Dielectric* other = reinterpret_cast<Dielectric*>(mat);
        ior = other->ior;
    };

    __host__ void teardown() { }

    __device__ NOINLINE bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const { 
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

class Emissive : public MaterialCore {
public:

    __host__ Emissive(const Color& albedo, const float brightness = 35.0f) 
        : texture(ConstantTexture(albedo).build()), brightness(brightness) {}

    template<typename T>
    __host__ Emissive(const T& texture, const float brightness = 35.0f) 
        : texture(texture.build()), brightness(brightness) {}

    __device__ Emissive(Texture* texture, float brightness) 
        : texture(texture), brightness(brightness) { }

    __device__ void update(MaterialCore* mat) {
        Emissive* other = reinterpret_cast<Emissive*>(mat);
        texture = other->texture;
        brightness = other->brightness;
    };

    __host__ void teardown() { if (texture) cudaFree(texture); }

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const {
        if (!rec.front_face) return Color::black();
        return texture->sample(rec.uv, rec.p) * brightness;
    }

private:

    Texture* texture;
    float    brightness;

};

// ############################################################################
// ISOTROPIC
// ############################################################################

class Isotropic : public MaterialCore {
public:

    __host__ Isotropic(const Color& albedo) 
        : texture(ConstantTexture(albedo).build()) {}

    template<typename T>
    __host__ Isotropic(const T& texture) : texture(texture.build()) {}

    __device__ Isotropic(Texture* texture) : texture(texture) { }

    __device__ void update(MaterialCore* mat) {
        Isotropic* other = reinterpret_cast<Isotropic*>(mat);
        texture = other->texture;
    };

    __host__ void teardown() { if (texture) cudaFree(texture); }

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const { 
        srec.atten = texture->sample(rec.uv, rec.p);
        srec.pdf   = PDF::sphere();
        srec.skip_pdf = false;
        return true;
    }

    __device__ Color evaluate(
        const HitRecord& rec, 
        const Vector3& view_dir, 
        const Vector3& light_dir
    ) const {
        return texture->sample(rec.uv, rec.p) * (1.0f / PI4);
    }

private:

    Texture* texture;
};

// ############################################################################
// MAIN MATERIAL CLASS
// ############################################################################

enum class MaterialType : uint8_t { 
    Null, Lambertian, PBR, Dielectric, Emissive, Isotropic
};

class Material {

private:

    MaterialType type;
    union {
        Lambertian* mat_lambertian;
        PBR*        mat_pbr;
        Dielectric* mat_dielectric;
        Emissive*   mat_emissive;
        Isotropic*  mat_isotropic;
    };

public:

    // CONSTRUCTORS ===========================================================

    __host__ Material() : type(MaterialType::Null) { }
    __host__ Material(MaterialType type) : type(type) { }

    __device__ Material(
        MaterialType type,
        MaterialCore* mat
    ) : type(type) { 
        switch (type) {
            case MaterialType::Null: return;
            case MaterialType::Lambertian: 
                mat_lambertian = reinterpret_cast<Lambertian*>(mat); break;
            case MaterialType::Dielectric: 
                mat_dielectric = reinterpret_cast<Dielectric*>(mat); break;
            case MaterialType::Emissive:   
                mat_emissive = reinterpret_cast<Emissive*>(mat); break;
            case MaterialType::Isotropic:  
                mat_isotropic = reinterpret_cast<Isotropic*>(mat); break;
            case MaterialType::PBR:        
                mat_pbr = reinterpret_cast<PBR*>(mat); break;
        }
    }

    // UTILITIES ==============================================================

    __host__ MaterialType material_type() const { return type; }

    // LAMBERTIAN =============================================================

    template<typename T>
    __host__ static Material lambertian(const T& texture) {
        Material m(MaterialType::Lambertian); 
        m.mat_lambertian = device_build<Lambertian>(texture.build());
        return m;
    }

    __host__ static Material lambertian(const Color& albedo) {
        return Material::lambertian(ConstantTexture(albedo));
    }

    // PBR METAL ROUGHNESS ====================================================

    template<typename T>
    __host__ static Material pbr(
        const T& albedo,
        const T& roughness,
        const T& metallic,
        const T& emission,
        const T& normal
    ) {
        Material m(MaterialType::PBR); 
        m.mat_pbr = device_build<PBR>(
            albedo.build(), roughness.build(), metallic.build(), 
            emission.build(), normal.build()
        );
        return m;
    }

    // DIELECTRIC =============================================================

    __host__ static Material dielectric(float ior = 1.5f) {
        Material m(MaterialType::Dielectric); 
        m.mat_dielectric = device_build<Dielectric>(ior);
        return m;
    }

    // EMISSIVE ===============================================================

    template<typename T>
    __host__ static Material emissive(
        const T& texture,
        const float brightness = 35.0f
    ) {
        Material m(MaterialType::Emissive); 
        m.mat_emissive = device_build<Emissive>(texture.build(), brightness);
        return m;
    }

    __host__ static Material emissive(
        const Color& albedo, 
        const float brightness = 35.0f
    ) {
        return Material::emissive(ConstantTexture(albedo), brightness);
    }

    // ISOTROPIC ==============================================================

    template<typename T>
    __host__ static Material isotropic(const T& texture) {
        Material m(MaterialType::Isotropic);
        m.mat_isotropic = device_build<Isotropic>(texture.build());
        return m;
    }

    __host__ static Material isotropic(const Color& albedo) {
        return Material::isotropic(ConstantTexture(albedo));
    }

    // UPDATE =================================================================

    __host__ Material* build() const {
        MaterialCore* d_mat_ptr = nullptr;
        switch (type) {
            case MaterialType::Lambertian: d_mat_ptr = mat_lambertian; break;
            case MaterialType::Dielectric: d_mat_ptr = mat_dielectric; break;
            case MaterialType::Emissive:   d_mat_ptr = mat_emissive;   break;
            case MaterialType::Isotropic:  d_mat_ptr = mat_isotropic;  break;
            case MaterialType::PBR:        d_mat_ptr = mat_pbr;        break;
        }
        return device_build<Material>(type, d_mat_ptr);

    }

    __device__ void update(MaterialCore* mat) {
        switch (type) {
            case MaterialType::Null: return;
            case MaterialType::Lambertian: mat_lambertian->update(mat); break;
            case MaterialType::Dielectric: mat_dielectric->update(mat); break;
            case MaterialType::Emissive:   mat_emissive->update(mat);   break;
            case MaterialType::Isotropic:  mat_isotropic->update(mat);  break;
            case MaterialType::PBR:        mat_pbr->update(mat);        break;
        }
    }

    __device__ void teardown() {
        switch (type) {
            case MaterialType::Null: return;
            case MaterialType::Lambertian: mat_lambertian->teardown(); break;
            case MaterialType::Dielectric: mat_dielectric->teardown(); break;
            case MaterialType::Emissive:   mat_emissive->teardown();   break;
            case MaterialType::Isotropic:  mat_isotropic->teardown();  break;
            case MaterialType::PBR:        mat_pbr->teardown();        break;
        }
    }

    // SCATTER ================================================================

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const {
        switch (type) {
            case MaterialType::Lambertian: 
                return mat_lambertian->scatter(rec, ray, srec, gen);
            case MaterialType::Dielectric: 
                return mat_dielectric->scatter(rec, ray, srec, gen);
            case MaterialType::Isotropic:  
                return mat_isotropic->scatter(rec, ray, srec, gen);
            case MaterialType::PBR:        
                return mat_pbr->scatter(rec, ray, srec, gen);
            default:
                return false;
        }
    }

    // EVALUATE ===============================================================

    __device__ Color evaluate(
        const HitRecord& rec, 
        const Vector3& view_dir, 
        const Vector3& light_dir
    ) const {
        switch (type) {
            case MaterialType::Lambertian: 
                return mat_lambertian->evaluate(rec, view_dir, light_dir);
            case MaterialType::Isotropic:  
                return mat_isotropic->evaluate(rec, view_dir, light_dir);
            case MaterialType::PBR:        
                return mat_pbr->evaluate(rec, view_dir, light_dir);
            default:
                return Color::black();
        }
    }

    // EMITTED ================================================================

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const {
        switch (type) {
            case MaterialType::Emissive: 
                return mat_emissive->emitted(ray, rec);
            case MaterialType::PBR: 
                return mat_pbr->emitted(ray, rec);
            default: 
                return Color::black();
        }
    }
    
};

