#pragma once

#include "hittable/include/hittable/hittable.h"
#include "material/include/material/material.h"
#include "material/include/material/texture.h"

class ConstantMedium : public Hittable {
public:

    __host__ ConstantMedium(
        Hittable* boundary,
        float     density,
        Texture*  texture
    ) : boundary(boundary),
        neg_inv_density(1.0f / density),
        phase_function(IsoTropic(texture).build()) 
    { }

    __host__ ConstantMedium(
        Hittable*    boundary,
        float        density,
        const Color& albedo
    ) : boundary(boundary),
        neg_inv_density(1.0f / density),
        phase_function(IsoTropic(albedo).build())
    { }

    // bool hit(
    //     const Ray& r, 
    //     Interval   ray_t, 
    //     HitRecord& rec
    // ) const override {
    //     HitRecord rec1, rec2;

    //     if (!boundary->hit(r, Interval::universe(), rec1))
    //         return false;

    //     if (!boundary->hit(r, Interval(rec1.t + EPS, FMAX), rec2))
    //         return false;

    //     if (rec1.t < ray_t.min) rec1.t = ray_t.min;
    //     if (rec2.t > ray_t.max) rec2.t = ray_t.max;
    //     if (rec1.t >= rec2.t)   return false;
    //     if (rec1.t < 0.0f)      rec1.t = 0.0f;

    //     auto ray_length = r.direction().length();
    //     auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
    //     auto hit_distance = neg_inv_density * logf(random_double());

    //     if (hit_distance > distance_inside_boundary)
    //         return false;

    //     rec.t = rec1.t + hit_distance / ray_length;
    //     rec.p = r.at(rec.t);

    //     rec.n = Vector3(1.0f, 0.0f, 0.0f);
    //     rec.front_face = true;
    //     rec.mat = phase_function;

    //     return true;
    // }

private:

    Hittable* boundary;
    Material* phase_function;
    float     neg_inv_density;

};

