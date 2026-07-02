#pragma once

#include "hittable/include/hittable/hittable.h"
#include "material/include/material/material.h"
#include "material/include/material/texture.h"
#include "engine/include/engine/hash.h"

class ConstantMedium : public Hittable {
public:

    __host__ ConstantMedium(
        Hittable& bound_obj,
        float density,
        const Texture& texture
    ) : Hittable(bound_obj.xform, bound_obj.delta, Isotropic(texture).build()),
        boundary(bound_obj.build()),
        neg_inv_density(-1.0f / density)
    { bbox = bound_obj.build_bbox(); }

    __host__ ConstantMedium(
        Hittable& bound_obj,
        float density,
        const Color& albedo
    ) : Hittable(bound_obj.xform, bound_obj.delta, Isotropic(albedo).build()),
        boundary(bound_obj.build()),
        neg_inv_density(-1.0f / density)
    { bbox = bound_obj.build_bbox(); }

    __device__ ConstantMedium(
        Transform& xform,
        Transform& delta,
        Material*  mat, 
        Hittable*  boundary_ptr, 
        float      neg_inv_density_
    ) : Hittable(xform, delta, mat), 
        boundary(boundary_ptr), 
        neg_inv_density(neg_inv_density_) 
    { }

    __host__ Hittable* build() const override {
        return device_build<ConstantMedium>(
            xform, delta, material, boundary, neg_inv_density
        );
    }

    __host__ const AABB build_bbox() const override { return bbox; }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override {
        HitRecord rec1, rec2;

        if (!boundary->hit(ray, Interval(-FMAX, FMAX), rec1))
            return false;

        if (!boundary->hit(ray, Interval(rec1.t + EPS, FMAX), rec2))
            return false;

        if (rec1.t < ray_t.min) rec1.t = ray_t.min;
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;
        if (rec1.t >= rec2.t)   return false;
        if (rec1.t < 0.0f)      rec1.t = 0.0f;

        auto ray_length = ray.direction().length();
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
        auto hit_distance = neg_inv_density * logf(pcg_float(ray));

        if (hit_distance > distance_inside_boundary)
            return false;

        rec.p = ray.at(rec1.t + hit_distance / ray_length);
        rec.n = Vector3(1.0f, 0.0f, 0.0f);
        rec.front_face = true;
        rec.mat = material;

        return true;
    }

private:

    Hittable* boundary;
    float     neg_inv_density;

};

