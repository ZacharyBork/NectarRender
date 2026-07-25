#pragma once

#include "core/include/core.h"

enum class TextureType { CONSTANT, IMAGE };

struct TextureView {
public:

    TextureType type;

    uint8_t* d_texture_ptr;
    Color constant_color;
    size_t C, H, W;
    
    __device__ Color sample(Vector2 uv, const Vector3& p) const {
        switch (type) {
            case TextureType::CONSTANT: return constant_color;
            case TextureType::IMAGE:    return sample_image(uv, p);
        }
        return Color::black();
    };

private:

    __device__ Color sample_image(Vector2 uv, const Vector3& p) const {
        float u = Interval(0.0f, 1.0f).clamp(uv.u());
        float v = 1.0f - Interval(0.0f, 1.0f).clamp(uv.v());

        int i = (int)(u * W); if (i >= (int)W) i = (int)W - 1;
        int j = (int)(v * H); if (j >= (int)H) j = (int)H - 1;

        size_t pixel = j * W + i;

        Color col(
            (float)d_texture_ptr[0 * H * W + pixel],
            (float)d_texture_ptr[1 * H * W + pixel],
            (float)d_texture_ptr[2 * H * W + pixel]
        );
        return col / 255.0f;
    }

};

class Texture {
public:

    TextureType type = TextureType::CONSTANT;

    /* CONSTRUCTORS / DESTRUCTORS */

    __host__ __device__ Texture() {}
    __host__ Texture(TextureType type) : type(type) {}
    __host__ ~Texture() = default;

    __host__ Texture(const Texture&) = delete;
    __host__ Texture& operator=(const Texture&) = delete;

    __host__ Texture(Texture&& other) noexcept
        : type(other.type), 
          texture_path(std::move(other.texture_path)),
          d_texture_ptr(other.d_texture_ptr), 
          constant_color(other.constant_color),
          C(other.C), H(other.H), W(other.W)
    {
        other.d_texture_ptr = nullptr;
    }

    __host__ Texture& operator=(Texture&& other) noexcept {
        if (this != &other) {
            teardown();
            C = other.C; H = other.H; W = other.W;
            type           = other.type;
            texture_path   = std::move(other.texture_path);
            d_texture_ptr  = other.d_texture_ptr;
            constant_color = other.constant_color;
            other.d_texture_ptr = nullptr;
        }
        return *this;
    }

    /* STATIC METHODS */

    __host__ static std::shared_ptr<Texture> from_color(const Color& color) {
        Texture t(TextureType::CONSTANT);
        t.constant_color = color;
        t.C = 3UL;
        return std::make_shared<Texture>(std::move(t));
    }

    __host__ static std::shared_ptr<Texture> from_color(
        float r, float g, float b
    ) {
        return Texture::from_color(Color(r, g, b));
    }

    __host__ static std::shared_ptr<Texture> from_image(
        std::string  filepath,
        uintptr_t    host_ptr, 
        const size_t channels,
        const size_t height,
        const size_t width
    ) { 
        Texture t(TextureType::IMAGE);
        t.texture_path = filepath.c_str();
        t.C = channels; t.H = height, t.W = width;
        size_t n_elements = t.C * t.H * t.W;

        uint8_t* tex_ptr;
        CUDAMemory::allocate<uint8_t>(tex_ptr, n_elements);
        CUDAMemory::copy<uint8_t>(
            tex_ptr, reinterpret_cast<uint8_t*>(&host_ptr), n_elements
        );
        t.d_texture_ptr = tex_ptr;
        return std::make_shared<Texture>(std::move(t));
    }
    
    /* UTILITIES */

    __host__ void teardown() { if (d_texture_ptr) cudaFree(d_texture_ptr); }

    __host__ TextureView view() const {
        return TextureView{ type, d_texture_ptr, constant_color, C, H, W };
    }

private:

    std::string texture_path = "";
    uint8_t* d_texture_ptr = nullptr;
    Color constant_color = Color::black();
    size_t C = 0UL, H = 0UL, W = 0UL;

};



