#pragma once

#include "core/include/core.h"

// ############################################################################
// ABSTRACT PARENT
// ############################################################################

class Texture {
public:

    __host__ __device__ Texture() {}
    __host__ __device__ virtual ~Texture() = default;
    __device__ virtual Color sample(Vector2 uv, const Vector3& p) const = 0;

    __host__ virtual Texture* build() const = 0;
};

// ############################################################################
// CONSTANT TEXTURE
// ############################################################################

class ConstantTexture : public Texture {
public:

    __host__ ConstantTexture(
        float r, float g, float b
    ) : albedo(Color(r, g, b)) { }

    __host__ __device__ ConstantTexture(const Color& albedo) 
        : albedo(albedo) { }

    __host__ Texture* build() const {
        return device_build<ConstantTexture>(albedo);
    }

    __device__ Color sample(Vector2 uv, const Vector3& p) const override {
        return albedo;
    }

private:

    Color albedo;

};

// ############################################################################
// CHECKER TEXTURE
// ############################################################################

class CheckerTexture : public Texture {
public:

    __host__ __device__ CheckerTexture(
        const Color& color1,
        const Color& color2,
        float scale
    ) : color1(color1), color2(color2), inv_scale(1.0f / (scale + FMIN)) { }
    
    __host__ Texture* build() const {
        return device_build<CheckerTexture>(color1, color2, inv_scale);
    }

    __device__ Color sample(Vector2 uv, const Vector3& p) const override {
        int x = int(floorf(inv_scale * p.x()));
        int y = int(floorf(inv_scale * p.y()));
        int z = int(floorf(inv_scale * p.z()));
        bool isEven = (x + y + z) % 2 == 0;
        return isEven ? color1 : color2;
    }

private:

    Color color1, color2;
    float inv_scale;

};
