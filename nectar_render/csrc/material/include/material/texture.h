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

    __host__ ConstantTexture(float r, float g, float b) 
        : albedo(Color(r, g, b)) { }

    __host__ __device__ ConstantTexture(const Color& albedo) 
        : albedo(albedo) { }

    __host__ Texture* build() const {
        return device_build<ConstantTexture>(albedo);
    }

    __device__ Color sample(Vector2 uv, const Vector3& p) const override {
        return albedo;
    }

    __device__ Color color() { return albedo; }

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

// ############################################################################
// IMAGE TEXTURE
// ############################################################################

class ImageTexture : public Texture {
public:

    __host__ ImageTexture(
        uintptr_t    host_ptr, 
        const size_t channels,
        const size_t height,
        const size_t width
    ) : C(channels), H(height), W(width) { 
        cudaMalloc(&device_ptr, n_bytes());
        cudaMemcpy(
            reinterpret_cast<void*>(device_ptr), 
            reinterpret_cast<void*>(host_ptr), 
            n_bytes(), cudaMemcpyHostToDevice
        );
    }

    __device__ ImageTexture(uint8_t* ptr, size_t c, size_t h, size_t w) 
        : device_ptr(ptr), C(c), H(h), W(w) {}

    __host__ __device__ size_t n_bytes() const {
        return C * H * W * sizeof(uint8_t);
    }

    __host__ Texture* build() const {
        return device_build<ImageTexture>(device_ptr, C, H, W);
    }

    __device__ Color sample(Vector2 uv, const Vector3& p) const override {
        if (H <= 0) return Color::purple();

        float u = Interval(0.0f, 1.0f).clamp(uv.u());
        float v = 1.0f - Interval(0.0f, 1.0f).clamp(uv.v());

        int i = (int)(u * W); if (i >= (int)W) i = (int)W - 1;
        int j = (int)(v * H); if (j >= (int)H) j = (int)H - 1;

        size_t pixel = j * W + i;

        Color col(
            (float)device_ptr[0 * H * W + pixel],
            (float)device_ptr[1 * H * W + pixel],
            (float)device_ptr[2 * H * W + pixel]
        );
        return col / 255.0f;
    }

private:

    uint8_t* device_ptr;
    size_t C;
    size_t H;
    size_t W;

};

// ############################################################################
// NOISE TEXTURE
// ############################################################################

class NoiseTexture : public Texture {
public:

    __host__ NoiseTexture() : NoiseTexture(1.0f)  { }

    __host__ NoiseTexture(
        float scale, 
        int iterations = 7, 
        uint32_t seed = 42u
    ): perlin(Perlin(seed).build()), scale(scale), iters(iterations) { }

    __device__ NoiseTexture(Perlin* p, float scale, int iterations) 
        : perlin(p), scale(scale), iters(iterations) { }

    __host__ Texture* build() const {
        return device_build<NoiseTexture>(perlin, scale, iters);
    }

    __device__ Color sample(Vector2 uv, const Vector3& p) const override {
        float n = perlin->turb(p, iters) * 10.0f;
        return Color::white() * 0.5f * (1.0f + sinf(scale * p.z() + n));
    }

private:

    Perlin* perlin = nullptr;
    
    float scale;
    int iters;

};

