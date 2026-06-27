#pragma once

#include "core/include/core.h"
#include "engine/include/engine/ray.h"

// ############################################################################
// ABSTRACT PARENT
// ############################################################################

class Light { public: __host__ __device__ Light() {} };

// ############################################################################
// SKYLIGHT
// ############################################################################

class SkyLight : public Light {
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
