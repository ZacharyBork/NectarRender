#pragma once

#include "core/include/core.h"
#include "hittable/include/hittable/hittable.h"

// ABSTRACT PARENT ============================================================

class PDF {
public:

    Vector3 direction;
    float value;

    __host__ __device__ virtual ~PDF() { }
    
    __device__ void init(Generator& gen) { 
        direction = generate(gen);
        value = get_value(direction); 
    }

    __device__ virtual float get_value(const Vector3& direction) const = 0;
    __device__ virtual Vector3 generate(Generator& gen) const = 0;

};

// SPHERE =====================================================================

class SpherePDF : public PDF {
public:
    __device__ SpherePDF(Generator& gen) { init(gen); }

    __device__ float get_value(const Vector3& direction) const override {
        return 1.0f / PI4;
    }

    __device__ Vector3 generate(Generator& gen) const override { 
        return random_unit_vector(gen);
    }
};

// COSINE =====================================================================

class CosinePDF : public PDF {
public:
    __device__ CosinePDF(const Vector3& w, Generator& gen) 
        : uvw(w) { init(gen); }

    __device__ float get_value(const Vector3& direction) const override {
        auto cosine_theta = dot(normalize(direction), uvw.w());
        return fmaxf(0.0f, cosine_theta / PI);
    }

    __device__ Vector3 generate(Generator& gen) const override {
        return uvw.transform(random_cosine_direction(gen));
    }

private:

    ONB uvw;

};

// HITTABLE ===================================================================

template<typename HittableType>
class HittablePDF : public PDF {
public:

    __device__ HittablePDF(
        HittableType** objects, 
        const Vector3& origin,
        Generator& gen
    ) : objects(objects), origin(origin) { init(gen); }

    __device__ float get_value(const Vector3& direction) const override {
        return objects[0]->pdf_value(origin, direction);
    }

    __device__ Vector3 generate(Generator& gen) const override {
        return objects[0]->random(origin, gen);
    }

private:

    HittableType** objects;
    Vector3 origin;
    
};

// MIXTURE ====================================================================

class MixturePDF : public PDF {
public:

    __device__ MixturePDF(PDF* p0, PDF* p1, Generator& gen) 
        : p{ p0, p1 } { init(gen); }

    __device__ float get_value(const Vector3& direction) const override {
        return 0.5f * p[0]->get_value(direction) 
             + 0.5f * p[1]->get_value(direction);
    }

    __device__ Vector3 generate(Generator& gen) const override {
        if (gen.random_float() < 0.5f) return p[0]->generate(gen);
        else return p[1]->generate(gen);
    }

private:

    PDF* p[2];

};

