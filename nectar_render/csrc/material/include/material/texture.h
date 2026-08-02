#pragma once

#include "stb_image.h"
#include "core/include/core.h"

enum class TextureType { CONSTANT, IMAGE };

__host__ inline std::vector<uint8_t> load_image_uint8(
    const std::string& filepath, int& C, int& H, int& W
) {
    int channels_in_file;
    unsigned char* data = stbi_load(filepath.c_str(), &W, &H, &C, 0);
    if (!data) {
        throw std::runtime_error(std::string("Failed to load image: ") 
            + stbi_failure_reason());
    }

    const size_t hw = static_cast<size_t>(W) * H;
    std::vector<uint8_t> chw(hw * C);

    for (size_t c = 0; c < C; ++c)
        for (size_t i = 0; i < hw; ++i)
            chw[c * hw + i] = data[i * 3 + c];

    stbi_image_free(data);
    return chw;
}

__host__ inline std::vector<float> load_image_float(
    const std::string& filepath, int& C, int& H, int& W
) {
    float* data = stbi_loadf(filepath.c_str(), &W, &H, &C, 0);
    if (!data) {
        throw std::runtime_error(std::string("Failed to load HDRI: ") 
            + stbi_failure_reason());
    }

    const size_t hw = static_cast<size_t>(W) * static_cast<size_t>(H);
    const size_t n_channels = static_cast<size_t>(C);
    std::vector<float> chw(hw * n_channels);

    for (size_t c = 0; c < n_channels; c++)
        for (size_t i = 0; i < hw; i++)
            chw[c * hw + i] = data[i * n_channels + c];

    stbi_image_free(data);
    return chw;
}

struct TextureView {
public:

    TextureType type;
    uint8_t*    d_tex_ptr;
    Color       constant_color;
    size_t      C, H, W;
    
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

        int r = 0 * H * W + pixel;
        int g = 1 * H * W + pixel;
        int b = 2 * H * W + pixel;

        return Color(
            (float)d_tex_ptr[r], (float)d_tex_ptr[g], (float)d_tex_ptr[b]
        ) / 255.0f;
    }

};

class Texture {
public:

    TextureType type = TextureType::CONSTANT;
    std::string filepath = "";

    uint8_t* d_texture_ptr = nullptr;
    Color constant_color = Color::black();
    size_t C = 0UL, H = 0UL, W = 0UL;

    /* CONSTRUCTORS / DESTRUCTORS */

    __host__ __device__ Texture() {}
    __host__ Texture(TextureType type) : type(type) {}
    __host__ ~Texture() { if (d_texture_ptr) cudaFree(d_texture_ptr); };

    __host__ Texture(const Texture&) = delete;
    __host__ Texture& operator=(const Texture&) = delete;

    __host__ Texture(Texture&& other) noexcept
        : type(other.type), 
          filepath(std::move(other.filepath)),
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
            filepath       = std::move(other.filepath);
            d_texture_ptr  = other.d_texture_ptr;
            constant_color = other.constant_color;
            other.d_texture_ptr = nullptr;
        }
        return *this;
    }

    /* STATIC METHODS */

    __host__ static std::shared_ptr<Texture> black() {
        return Texture::from_color(Color::black());
    }

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
        std::string filepath
    ) { 
        Texture t(TextureType::IMAGE);
        t.filepath = filepath.c_str();
        
        int C, H, W;
        std::vector<uint8_t> image_data = load_image_uint8(filepath, C, H, W);
        uint8_t* h_tex_ptr = image_data.data();
        t.C = (size_t)C; t.H = (size_t)H, t.W = (size_t)W;
        size_t n_elements = t.C * t.H * t.W;

        uint8_t* d_tex_ptr;
        CUDAMemory::allocate<uint8_t>(d_tex_ptr, n_elements);
        CUDAMemory::copy<uint8_t>(d_tex_ptr, h_tex_ptr, n_elements);
        t.d_texture_ptr = d_tex_ptr;
        return std::make_shared<Texture>(std::move(t));
    }

    /* UTILITIES */

    __host__ void teardown() { 
        if (d_texture_ptr) CUDAMemory::free(d_texture_ptr); 
    }

    __host__ TextureView view() const {
        return TextureView{ 
            type, d_texture_ptr, constant_color, C, H, W
        };
    }


};



