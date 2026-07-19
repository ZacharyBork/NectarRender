#pragma once

#include "core/include/core.h"

// SPHERE =====================================================================

class SpherePDF {
public:
    __device__ SpherePDF() {  }

    __device__ float value(const Vector3& direction) const {
        return 1.0f / PI4;
    }

    __device__ Vector3 generate(Generator& gen) const { 
        return random_unit_vector(gen);
    }
};

// COSINE =====================================================================

class CosinePDF {
public:
    __device__ CosinePDF(const Vector3& w) : uvw(w) { }

    __device__ float value(const Vector3& direction) const {
        auto cosine_theta = dot(normalize(direction), uvw.w());
        return fmaxf(0.0f, cosine_theta / PI);
    }

    __device__ Vector3 generate(Generator& gen) const {
        return uvw.transform(random_cosine_direction(gen));
    }

private:

    ONB uvw;

};

// HITTABLE ===================================================================

class Hittable;

template<typename HittableType>
class HittablePDF {
public:

    __device__ HittablePDF(
        HittableType** objects, 
        const Vector3& origin
    ) : objects(objects), origin(origin) { }

    __device__ float value(const Vector3& direction) const {
        return objects[0]->pdf_value(origin, direction);
    }

    __device__ Vector3 generate(Generator& gen) const {
        return objects[0]->random(origin, gen);
    }

private:

    HittableType** objects;
    Vector3 origin;
    
};

// PHONG ======================================================================

class PhongPDF {
public:
    __device__ PhongPDF(const Vector3& reflect_dir, float shininess)
        : uvw(reflect_dir), exponent(shininess) {}

    __device__ float value(const Vector3& direction) const {
        float cosine = dot(normalize(direction), uvw.w());
        if (cosine <= 0.0f) return 0.0f;
        return (exponent + 1.0f) / PI2 * powf(cosine, exponent);
    }

    __device__ Vector3 generate(Generator& gen) const {
        float u1 = gen.random_float(), u2 = gen.random_float();
        float cos_theta = powf(u1, 1.0f / (exponent + 1.0f));
        float sin_theta = sqrtf(1.0f - cos_theta * cos_theta);
        float phi = PI2 * u2;
        Vector3 local(cosf(phi) * sin_theta, sinf(phi) * sin_theta, cos_theta);
        return uvw.transform(local);
    }

private:

    ONB uvw;
    float exponent;

};

// PBR LOBE ===================================================================

__device__ inline float ggx_distribution(float n_dot_h, float alpha) {
    float a2 = alpha * alpha;
    float d  = n_dot_h * n_dot_h * (a2 - 1.0f) + 1.0f;
    return a2 / fmaxf(PI * d * d, 1e-8f);
}

__device__ inline float smith_ggx_g1(float n_dot_x, float alpha) {
    float a2 = alpha * alpha;
    return 2.0f * n_dot_x / (n_dot_x + sqrtf(a2 + (1.0f - a2) * n_dot_x * n_dot_x));
}

__device__ inline float smith_ggx_geometry(float n_dot_l, float n_dot_v, float alpha) {
    return smith_ggx_g1(n_dot_l, alpha) * smith_ggx_g1(n_dot_v, alpha);
}

class PBRLobePDF {
public:

    __device__ PBRLobePDF(
        const Vector3& n, 
        const Vector3& view_dir, 
        float roughness, 
        float spec_prob
    ) : uvw(n), 
        view_dir(view_dir), 
        alpha(fmaxf(roughness * roughness, 1e-3f)), 
        spec_prob(spec_prob) 
    { }

    __device__ float value(const Vector3& direction) const {
        float diffuse_pdf = fmaxf(0.0f, dot(normalize(direction), uvw.w()) / PI);

        Vector3 h = normalize(view_dir + direction);
        float n_dot_h = fmaxf(dot(uvw.w(), h), 0.0f);
        float v_dot_h = fmaxf(dot(view_dir, h), 1e-4f);
        float specular_pdf = (ggx_distribution(n_dot_h, alpha) * n_dot_h) / (4.0f * v_dot_h);

        return (1.0f - spec_prob) * diffuse_pdf + spec_prob * specular_pdf;
    }

    __device__ Vector3 generate(Generator& gen) const {
        if (gen.random_float() < spec_prob) {
            float u1 = gen.random_float(), u2 = gen.random_float();
            float cos_theta = sqrtf((1.0f - u1) / (1.0f + (alpha * alpha - 1.0f) * u1));
            float sin_theta = sqrtf(fmaxf(0.0f, 1.0f - cos_theta * cos_theta));
            float phi = PI2 * u2;
            Vector3 h = uvw.transform(Vector3(sin_theta * cosf(phi), sin_theta * sinf(phi), cos_theta));
            return normalize(2.0f * dot(view_dir, h) * h - view_dir);
        }
        return uvw.transform(random_cosine_direction(gen));
    }

private:
    ONB uvw; 
    Vector3 view_dir; 
    float alpha; 
    float spec_prob;
};

// PDF RECORD =================================================================

enum class PDFType : uint8_t { 
    Cosine, Sphere, Phong, Hittable, Mixture, PBRLobe 
};

struct PDF {
    PDFType type;
    union {
        CosinePDF   cosine_pdf;
        SpherePDF   sphere_pdf;
        PhongPDF    phong_pdf;
        HittablePDF<Hittable> hittable_pdf;
        PBRLobePDF pbr_lobe_pdf;
    };

    __device__ PDF() : type(PDFType::Sphere), sphere_pdf() {}

    __device__ static PDF sphere() {
        PDF r; 
        r.type = PDFType::Sphere; 
        r.sphere_pdf = SpherePDF();
        return r;
    }

    __device__ static PDF cosine(const Vector3& n) {
        PDF r; 
        r.type = PDFType::Cosine; 
        r.cosine_pdf = CosinePDF(n);
        return r;
    }
    __device__ static PDF phong(
        const Vector3& reflect_dir, 
        float exponent
    ) {
        PDF r; 
        r.type = PDFType::Phong; 
        r.phong_pdf = PhongPDF(reflect_dir, exponent);
        return r;
    }
    __device__ static PDF hittable(
        Hittable** objs, 
        const Vector3& origin
    ) {
        PDF r; 
        r.type = PDFType::Hittable; 
        r.hittable_pdf = HittablePDF(objs, origin);
        return r;
    }

    __device__ static PDF pbr_lobe(
        const Vector3& n, 
        const Vector3& view_dir, 
        float roughness, 
        float spec_prob
    ) {
        PDF r; 
        r.type = PDFType::PBRLobe; 
        r.pbr_lobe_pdf = PBRLobePDF(n, view_dir, roughness, spec_prob);
        return r;
    }

    __device__ float value(const Vector3& direction) const {
        switch (type) {
            case PDFType::Cosine:   return cosine_pdf.value(direction);
            case PDFType::Sphere:   return sphere_pdf.value(direction);
            case PDFType::Phong:    return phong_pdf.value(direction);
            case PDFType::Hittable: return hittable_pdf.value(direction);
            case PDFType::PBRLobe:  return pbr_lobe_pdf.value(direction);
        }
        return 0.0f;
    }

    __device__ Vector3 generate(Generator& gen) const {
        switch (type) {
            case PDFType::Cosine:   return cosine_pdf.generate(gen);
            case PDFType::Sphere:   return sphere_pdf.generate(gen);
            case PDFType::Phong:    return phong_pdf.generate(gen);
            case PDFType::Hittable: return hittable_pdf.generate(gen);
            case PDFType::PBRLobe:  return pbr_lobe_pdf.generate(gen);
        }
        return Vector3(0.0f, 0.0f, 1.0f);
    }
};

// MIXTURE ====================================================================

class MixturePDF : public PDF {
public:
    __device__ MixturePDF() {}

    __device__ MixturePDF(const PDF& p0, const PDF& p1)
        : pdf0(p0), pdf1(p1) {}

    __device__ float value(const Vector3& direction) const {
        return 0.5f * pdf0.value(direction) 
             + 0.5f * pdf1.value(direction);
    }

    __device__ Vector3 generate(Generator& gen) const {
        if (gen.random_float() < 0.5f) return pdf0.generate(gen);
        else return pdf1.generate(gen);
    }

private:

    PDF pdf0, pdf1;

};
