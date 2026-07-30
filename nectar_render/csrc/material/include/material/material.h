#pragma once

#include <cstring>
#include <assert.h>
#include <cuda_runtime.h>

#include "core/include/core.h"

#include "engine/include/engine/ray.h"
#include "engine/include/engine/pdf.h"
#include "hittable/include/hittable/hit_record.h"

#include "material/include/material/texture.h"

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

    __device__ Lambertian(TextureView albedo) : albedo(albedo) { }

    __device__ void update(MaterialCore* mat) {
        Lambertian* other = reinterpret_cast<Lambertian*>(mat);
        albedo = other->albedo;
    };
    
    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const { 
        srec.atten = albedo.sample(rec.uv, rec.p);
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
        return albedo.sample(rec.uv, rec.p) * (cos_theta / PI);
    }

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const { return Color::black(); }

private:

    TextureView albedo;
    
};

// ############################################################################
// PBR METAL ROUGHNESS
// ############################################################################

class PBR : public MaterialCore {
public:

    __device__ PBR(
        TextureView albedo,
        TextureView roughness,
        TextureView metallic,
        TextureView emission,
        TextureView normal
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

    __device__ NOINLINE Vector3 get_shading_normal(
        const HitRecord& rec
    ) const {
        Color tex_n = normal_tex.sample(rec.uv, rec.p);
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
        Color   albedo   = albedo_tex.sample(rec.uv, rec.p);
        float   metallic = metallic_tex.sample(rec.uv, rec.p).r();

        Color F0 = Color(0.04f) * (1.0f - metallic) + albedo * metallic;
        float spec_prob = fminf(
            0.9f, fmaxf(0.1f, fmaxf(F0.r(), fmaxf(F0.g(), F0.b())))
        );

        srec.atten    = Color::white();
        srec.skip_pdf = false;
        srec.pdf      = PDF::pbr_lobe(
            n, normalize(-ray.direction()), 
            roughness_tex.sample(rec.uv, rec.p).r(), 
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

        Color albedo    = albedo_tex.sample(rec.uv, rec.p);
        float roughness = roughness_tex.sample(rec.uv, rec.p).r();
        float metallic  = metallic_tex.sample(rec.uv, rec.p).r();
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
        return emission_tex.sample(rec.uv, rec.p);
    }

private:

    TextureView albedo_tex;
    TextureView roughness_tex;
    TextureView metallic_tex;
    TextureView emission_tex;
    TextureView normal_tex;

};

// ############################################################################
// DIELECTRIC
// ############################################################################

class Dielectric : public MaterialCore {
public:

    __device__ Dielectric(float ior = 1.5f) : ior(ior) {}

    __device__ void update(MaterialCore* mat) {
        Dielectric* other = reinterpret_cast<Dielectric*>(mat);
        ior = other->ior;
    };

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

    __device__ Color evaluate(
        const HitRecord& rec, 
        const Vector3& view_dir, 
        const Vector3& light_dir
    ) const { return Color::black(); }

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const { return Color::black(); }

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

    __device__ Emissive(TextureView texture, float brightness) 
        : texture(texture), brightness(brightness) { }

    __device__ void update(MaterialCore* mat) {
        Emissive* other = reinterpret_cast<Emissive*>(mat);
        texture = other->texture;
        brightness = other->brightness;
    };

    __device__ bool scatter(
        HitRecord& rec, 
        Ray& ray, 
        ScatterRecord& srec, 
        Generator& gen
    ) const { return false; }

    __device__ Color evaluate(
        const HitRecord& rec, 
        const Vector3& view_dir, 
        const Vector3& light_dir
    ) const { return Color::black(); }

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const {
        if (!rec.front_face) return Color::black();
        return texture.sample(rec.uv, rec.p) * brightness;
    }

private:

    TextureView texture;
    float brightness;

};

// ############################################################################
// ISOTROPIC
// ############################################################################

class Isotropic : public MaterialCore {
public:

    __device__ Isotropic(TextureView texture) : texture(texture) { }

    __device__ void update(MaterialCore* mat) {
        Isotropic* other = reinterpret_cast<Isotropic*>(mat);
        texture = other->texture;
    };

    __device__ bool scatter(
        HitRecord& rec,
        Ray& ray,
        ScatterRecord& srec,
        Generator& gen
    ) const { 
        srec.atten = texture.sample(rec.uv, rec.p);
        srec.pdf   = PDF::sphere();
        srec.skip_pdf = false;
        return true;
    }

    __device__ Color evaluate(
        const HitRecord& rec, 
        const Vector3& view_dir, 
        const Vector3& light_dir
    ) const {
        return texture.sample(rec.uv, rec.p) * (1.0f / PI4);
    }

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const { return Color::black(); }

private:

    TextureView texture;

};

// ############################################################################
// MAIN MATERIAL CLASS
// ############################################################################

enum class MaterialType : uint8_t { 
    Null, Lambertian, PBR, Dielectric, Emissive, Isotropic
};

#define FOR_EACH_MATERIAL_TYPE(X) \
    X(Lambertian, mat_lambertian) \
    X(PBR,        mat_pbr)        \
    X(Dielectric, mat_dielectric) \
    X(Emissive,   mat_emissive)   \
    X(Isotropic,  mat_isotropic)

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

    struct TextureTracker {
        static constexpr uint8_t MAX_TEXTURES = 6u;
        std::shared_ptr<Texture> textures[MAX_TEXTURES];
        uint8_t count = 0u;
    };
    TextureTracker* tracker = nullptr;

    __host__ void track(std::shared_ptr<Texture> texture) {
        if (!tracker) tracker = new TextureTracker();
        assert(tracker->count < TextureTracker::MAX_TEXTURES);
        tracker->textures[tracker->count++] = std::move(texture);
    }

    __host__ void track(std::vector<std::shared_ptr<Texture>> textures) {
        for (std::shared_ptr<Texture> texture : textures) track(texture);
    }

    __host__ void* core_ptr() const {
        switch (type) {
            case MaterialType::Lambertian: return mat_lambertian;
            case MaterialType::Dielectric: return mat_dielectric;
            case MaterialType::Emissive:   return mat_emissive;
            case MaterialType::Isotropic:  return mat_isotropic;
            case MaterialType::PBR:        return mat_pbr;
            default: return nullptr;
        }
    }

public:

    // CONSTRUCTORS ===========================================================

    __host__ ~Material() = default;
    __host__ Material(const Material&) = delete;

    __host__ Material(Material&& other) noexcept {
        std::memcpy(this, &other, sizeof(Material));
        other.type = MaterialType::Null;
        other.tracker = nullptr;
    }
    
    __host__ __device__ Material() : type(MaterialType::Null) { }
    __host__ __device__ Material(MaterialType type) : type(type) { }
    
    __device__ Material(MaterialType type, void* mat) : type(type) { 
        switch (type) {
            case MaterialType::Null: return;
            #define X(Name, Member) case MaterialType::Name: \
                Member = reinterpret_cast<Name*>(mat); break;
            FOR_EACH_MATERIAL_TYPE(X)
            #undef X
        }
    }

    // LAMBERTIAN =============================================================

    __host__ static Material lambertian(std::shared_ptr<Texture> texture) {
        Material m(MaterialType::Lambertian); 
        TextureView v = texture->view(); m.track(texture);
        m.mat_lambertian = device_build<Lambertian>(v);
        return m;
    }

    __host__ static Material lambertian(const Color& albedo) {
        return Material::lambertian(Texture::from_color(albedo));
    }

    // PBR METAL ROUGHNESS ====================================================

    __host__ static Material pbr(
        std::shared_ptr<Texture> albedo,
        std::shared_ptr<Texture> roughness,
        std::shared_ptr<Texture> metallic,
        std::shared_ptr<Texture> emission,
        std::shared_ptr<Texture> normal
    ) {
        Material m(MaterialType::PBR);

        TextureView v_albedo    = albedo->view();
        TextureView v_roughness = roughness->view();
        TextureView v_metallic  = metallic->view();
        TextureView v_emission  = emission->view();
        TextureView v_normal    = normal->view();

        m.track({ albedo, roughness, metallic, emission, normal });
        m.mat_pbr = device_build<PBR>(
            v_albedo, v_roughness, v_metallic, v_emission, v_normal
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

    __host__ static Material emissive(
        std::shared_ptr<Texture> texture,
        const float brightness = 35.0f
    ) {
        Material m(MaterialType::Emissive); 
        TextureView v = texture->view(); m.track(texture);
        m.mat_emissive = device_build<Emissive>(v, brightness);
        return m;
    }

    __host__ static Material emissive(
        const Color& albedo, 
        const float brightness = 35.0f
    ) {
        return Material::emissive(Texture::from_color(albedo), brightness);
    }

    // ISOTROPIC ==============================================================

    __host__ static Material isotropic(std::shared_ptr<Texture> texture) {
        Material m(MaterialType::Isotropic); 
        TextureView v = texture->view(); m.track(texture);
        m.mat_isotropic = device_build<Isotropic>(v);
        return m;
    }

    __host__ static Material isotropic(const Color& albedo) {
        return Material::isotropic(Texture::from_color(albedo));
    }

    // OPERATORS ==============================================================

    __host__ Material& operator=(Material&& other) noexcept {
        if (this != &other) {
            std::memcpy(this, &other, sizeof(Material));
            other.type = MaterialType::Null;
            other.tracker = nullptr;
        }
        return *this;
    }

    __host__ Material& operator=(const Material&) = delete;

    // UTILITIES ==============================================================

    __host__ MaterialType material_type() const { return type; }
    __host__ uint8_t texture_count() const { 
        return tracker ? tracker->count : 0u; 
    } 

    // UPDATE =================================================================

    __host__ Material* build() const {
        return device_build<Material>(type, core_ptr());
    }

    __host__ void teardown() {
        if (tracker) {
            for (int i = 0; i < tracker->count; i++)
                tracker->textures[i]->teardown();
            delete tracker;
            tracker = nullptr;
        }
        void* core = core_ptr();
        if (core) CUDAMemory::free(core);
    }
    
    __device__ void update(MaterialCore* mat) {
        switch (type) {
            #define X(Name, Member) case MaterialType::Name: \
                return Member->update(mat); break;
            FOR_EACH_MATERIAL_TYPE(X)
            #undef X
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
            default: return false;
            #define X(Name, Member) case MaterialType::Name: \
                return Member->scatter(rec, ray, srec, gen);
            FOR_EACH_MATERIAL_TYPE(X)
            #undef X
        }
    }

    // EVALUATE ===============================================================

    __device__ Color evaluate(
        const HitRecord& rec, 
        const Vector3& view_dir, 
        const Vector3& light_dir
    ) const {
        switch (type) {
            default: return Color::black();
            #define X(Name, Member) case MaterialType::Name: \
                return Member->evaluate(rec, view_dir, light_dir);
            FOR_EACH_MATERIAL_TYPE(X)
            #undef X
        }
    }

    // EMITTED ================================================================

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const {
        switch (type) {
            default: return Color::black();
            #define X(Name, Member) case MaterialType::Name: \
                return Member->emitted(ray, rec);
            FOR_EACH_MATERIAL_TYPE(X)
            #undef X
        }
    }
    
};

__host__ inline Material make_default_material() {
    return Material::lambertian(Color::purple());
}


