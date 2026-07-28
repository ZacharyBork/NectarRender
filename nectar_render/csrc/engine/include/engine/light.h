#pragma once

#include "core/include/core.h"
#include "engine/include/engine/ray.h"
#include "hittable/include/hittable/hittable.h"

// ############################################################################
// SKYLIGHT
// ############################################################################

class SkyLight {
public:

    __host__ SkyLight () 
        : start(Color(1.0f, 1.0f, 1.0f)), end(Color(0.5f, 0.7f, 1.0f)) { }

    __host__ SkyLight (
        const Color& start_color, 
        const Color& end_color
    ) : start(start_color), end(end_color) { }

    __host__ static SkyLight black() {
        return SkyLight(Color::black(), Color::black());
    }

    __device__ Color sample(const Ray& ray) {
        Vector3 unit_direction = normalize(ray.direction());
        float a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * start + a * end;
    }

private:

    Color start, end;

};

// ############################################################################
// HITTABLE LIGHTS
// ############################################################################

class Light : public Hittable { using Hittable::Hittable; };

class ObjectLight : public Light {
public:

    __host__ ObjectLight(
        Hittable& obj,
        float brightness,
        std::shared_ptr<Texture> texture
    ) : Light(obj.xform, obj.delta, Material::emissive(texture, brightness)),
        boundary(obj.build())
    { bbox = obj.build_bbox(); }

    __host__ ObjectLight(
        Hittable& obj,
        float brightness,
        const Color& albedo
    ) : Light(obj.xform, obj.delta, Material::emissive(albedo, brightness)),
        boundary(obj.build())
    { bbox = obj.build_bbox(); }

    __device__ ObjectLight(
        HittableBaseData data,
        Hittable*  boundary_ptr
    ) : Light(data), boundary(boundary_ptr) { }

    __host__ Hittable* build() const override {
        return to_device<ObjectLight>(boundary);
    }

    __host__ const AABB build_bbox() const override { return bbox; }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override {
        if (!boundary->hit(ray, Interval(-FMAX, FMAX), rec))
            return false;

        return true;
    }

    __device__ float pdf_value(
        const Vector3& origin, 
        const Vector3& direction
    ) const override {
        return boundary->pdf_value(origin, direction);
    }

    __device__ Vector3 random(
        const Vector3& origin,
        Generator& gen
    ) const override {
        return boundary->random(origin, gen);
    }

private:

    Hittable* boundary;

};


