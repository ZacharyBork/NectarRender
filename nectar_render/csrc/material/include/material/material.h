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

    __host__ bool operator==(const Lambertian& other) {
        return texture == other.texture;
    }
    
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

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const { return Color::black(); }

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

    __host__ bool operator==(const PBR& other) {
        return albedo_tex    == other.albedo_tex
            && roughness_tex == other.roughness_tex
            && metallic_tex  == other.metallic_tex
            && emission_tex  == other.emission_tex
            && normal_tex    == other.normal_tex;
    }

    __device__ NOINLINE Vector3 get_shading_normal(
        const HitRecord& rec
    ) const {
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

    __host__ bool operator==(const Dielectric& other) {
        return ior == other.ior;
    }

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

    __host__ bool operator==(const Emissive& other) {
        return texture    == other.texture
            && brightness == other.brightness;
    }

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

    __host__ bool operator==(const Isotropic& other) {
        return texture == other.texture;
    }

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

    __device__ Color emitted(
        const Ray&       ray,
        const HitRecord& rec
    ) const { return Color::black(); }

private:

    Texture* texture;
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

    static constexpr uint8_t MAX_TRACKED_RESOURCES = 6u;
    void* tracked_resources[MAX_TRACKED_RESOURCES] = { nullptr };
    uint8_t n_tracked_resources = 0u;

    __host__ void track(void* ptr) {
        assert(n_tracked_resources < MAX_TRACKED_RESOURCES);
        tracked_resources[n_tracked_resources++] = ptr;
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
    __host__ Material(const Material&) = default;

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

    __host__ Material(Material&& other) noexcept {
        std::memcpy(this, &other, sizeof(Material));
        other.type = MaterialType::Null;
        other.n_tracked_resources = 0;
    }

    // OPERATORS ==============================================================

    __host__ bool operator==(const Material& other) {
        if (type != other.type) return false;
        switch (type) {
            case MaterialType::Null: return true;
            #define X(Name, Member) case MaterialType::Name: \
                return Member == other.Member;
            FOR_EACH_MATERIAL_TYPE(X)
            #undef X
        }
        return false;
    }

    __host__ Material& operator=(Material&& other) noexcept {
        if (this != &other) {
            std::memcpy(this, &other, sizeof(Material));
            other.type = MaterialType::Null;
            other.n_tracked_resources = 0;
        }
        return *this;
    }

    __host__ Material& operator=(const Material&) = default;

    // UTILITIES ==============================================================

    __host__ MaterialType material_type() const { return type; }
    __host__ uint8_t resource_count() const { return n_tracked_resources; } 

    // LAMBERTIAN =============================================================

    template<typename T>
    __host__ static Material lambertian(const T& texture) {
        Material m(MaterialType::Lambertian); 
        Texture* t = texture.build(); m.track(t);
        m.mat_lambertian = device_build<Lambertian>(t);
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
        Texture* albedo_    = albedo.build();    m.track(albedo_);
        Texture* roughness_ = roughness.build(); m.track(roughness_);
        Texture* metallic_  = metallic.build();  m.track(metallic_);
        Texture* emission_  = emission.build();  m.track(emission_);
        Texture* normal_    = normal.build();    m.track(normal_);
        m.mat_pbr = device_build<PBR>(
            albedo_, roughness_, metallic_, emission_, normal_
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
        Texture* t = texture.build(); m.track(t);
        m.mat_emissive = device_build<Emissive>(t, brightness);
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
        Texture* t = texture.build(); m.track(t);
        m.mat_isotropic = device_build<Isotropic>(t);
        return m;
    }

    __host__ static Material isotropic(const Color& albedo) {
        return Material::isotropic(ConstantTexture(albedo));
    }

    // UPDATE =================================================================

    __host__ Material* build() const {
        return device_build<Material>(type, core_ptr());
    }

    __host__ void teardown() {
        for (int i = 0; i < n_tracked_resources; i++)
            if (tracked_resources[i]) cudaFree(tracked_resources[i]);
        n_tracked_resources = 0;

        void* core = core_ptr();
        if (core) cudaFree(core);
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




