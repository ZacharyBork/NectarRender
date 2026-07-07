#pragma once

#include "core/include/core.h"
// #include "hittable/include/hittable/hittable.h"

// ABSTRACT PARENT ============================================================

// class PDF {
// public:

//     Vector3 direction;
//     float value;

//     __host__ __device__ virtual ~PDF() { }

//     __device__ void init(Generator& gen) { 
//         direction = generate(gen);
//         value = get_value(direction); 
//     }

//     __device__ virtual float get_value(const Vector3& direction) const = 0;
//     __device__ virtual Vector3 generate(Generator& gen) const = 0;

// };

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

// PDF RECORD =================================================================

enum class PDFType : uint8_t { Cosine, Sphere, Phong, Hittable, Mixture };

struct PDF {
    PDFType type;
    union {
        CosinePDF   cosine_pdf;
        SpherePDF   sphere_pdf;
        PhongPDF    phong_pdf;
        HittablePDF<Hittable> hittable_pdf;
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

    __device__ float value(const Vector3& direction) const {
        switch (type) {
            case PDFType::Cosine:   return cosine_pdf.value(direction);
            case PDFType::Sphere:   return sphere_pdf.value(direction);
            case PDFType::Phong:    return phong_pdf.value(direction);
            case PDFType::Hittable: return hittable_pdf.value(direction);
        }
        return 0.0f;
    }

    __device__ Vector3 generate(Generator& gen) const {
        switch (type) {
            case PDFType::Cosine:   return cosine_pdf.generate(gen);
            case PDFType::Sphere:   return sphere_pdf.generate(gen);
            case PDFType::Phong:    return phong_pdf.generate(gen);
            case PDFType::Hittable: return hittable_pdf.generate(gen);
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
