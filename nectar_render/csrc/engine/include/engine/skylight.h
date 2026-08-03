#pragma once

#include "core/include/core.h"
#include "engine/include/engine/ray.h"
#include "material/include/material/texture.h"

struct SimpleSkylightConfig {
    Color start = Color(1.0f, 1.0f, 1.0f);
    Color end   = Color(0.5f, 0.7f, 1.0f);
};

struct HDRISkylightConfig {
    float rotation = 0.0f;
    float intensity = 1.0f;

    float* data_ptr = nullptr;
    int C = 0, H = 0, W = 0;

};


namespace Sky {

// ============================================================================
// SIMPLE
// ============================================================================

    class Simple {
    public:

        __device__ static Color sample(
            const Ray& ray,
            SimpleSkylightConfig cfg
        ) {
            float a = 0.5 * (ray.direction().y() + 1.0);
            return (1.0 - a) * cfg.start + a * cfg.end;
        }

    private:

        Color start, end;

    };

// ============================================================================
// HDRI
// ============================================================================

    struct HDRI {
    public:

        __host__ static void load_file(
            const std::string& filepath,
            HDRISkylightConfig& cfg
        ) {
            std::vector<float> image_data = load_image_float(
                filepath, cfg.C, cfg.H, cfg.W
            );
            size_t n_elements = (size_t)(cfg.C * cfg.H * cfg.W);
            CUDAMemory::allocate<float>(cfg.data_ptr, n_elements);
            CUDAMemory::copy<float>(
                cfg.data_ptr, image_data.data(), n_elements
            );
        }

        __device__ static Color sample(
            const Ray& ray,
            HDRISkylightConfig cfg
        ) {
            if (!cfg.data_ptr) return Color::black();
            Matrix3 rot = rotation_y(deg2rad(cfg.rotation));
            Vector3 dir = rot * normalize(ray.direction());
            Vector2 uv = hdri_uvs(dir);
            return sample_hdri(uv, cfg) * cfg.intensity;
        }

    private:

        __device__ static Vector2 hdri_uvs(const Vector3& dir) {
            float u = atan2f(dir.z(), dir.x()) / PI2 + 0.5f;
            float v = acosf(fminf(fmaxf(dir.y(), -1.0f), 1.0f)) / PI;
            return Vector2(u, v);
        }

        __device__ static Color sample_hdri(
            Vector2 uv, 
            HDRISkylightConfig cfg
        ) {
            float fx = uv.u() * (float)cfg.W - 0.5f;
            float fy = uv.v() * (float)cfg.H - 0.5f;

            int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
            int x1 = x0 + 1, y1 = y0 + 1;
            float tx = fx - (float)x0, ty = fy - (float)y0;

            x0 = ((x0 % (int)cfg.W) + (int)cfg.W) % (int)cfg.W;
            x1 = ((x1 % (int)cfg.W) + (int)cfg.W) % (int)cfg.W;
            y0 = fmaxf(0, fminf((int)cfg.H - 1, y0));
            y1 = fmaxf(0, fminf((int)cfg.H - 1, y1));

            auto texel = [&](int x, int y) {
                size_t hw = cfg.H * cfg.W;
                size_t idx = (size_t)y * cfg.W + (size_t)x;
                return Color(
                    cfg.data_ptr[0*hw+idx], 
                    cfg.data_ptr[1*hw+idx], 
                    cfg.data_ptr[2*hw+idx]
                );
            };

            Color c00 = texel(x0, y0), c10 = texel(x1, y0);
            Color c01 = texel(x0, y1), c11 = texel(x1, y1);

            Color top    = c00 * (1.0f - tx) + c10 * tx;
            Color bottom = c01 * (1.0f - tx) + c11 * tx;
            return top * (1.0f - ty) + bottom * ty;
        }
    };

}

// ============================================================================
// SKYLIGHT WRAPPER
// ============================================================================

enum class SkylightType{ Null, Simple, HDRI };

#define FOR_EACH_SKYLIGHT_TYPE(X) \
    X(Simple, cfg_simple)         \
    X(HDRI,   cfg_hdri)           \


class Skylight {
private:

    SkylightType type;
    union {
        SimpleSkylightConfig cfg_simple;
        HDRISkylightConfig   cfg_hdri;
    };

public:

    // CONSTRUCTORS ===========================================================

    __host__ ~Skylight() = default;
    __host__ Skylight(const Skylight&) = delete;

    __host__ __device__ Skylight() : type(SkylightType::Null) { }
    __host__ __device__ Skylight(SkylightType type) : type(type) { }

    __host__ Skylight(Skylight&& other) noexcept {
        std::memcpy(this, &other, sizeof(Skylight));
        other.type = SkylightType::Null;
    }
    
    // OPERATORS ==============================================================

    __host__ Skylight& operator=(Skylight&& other) noexcept {
        teardown();
        if (this != &other) {
            std::memcpy(this, &other, sizeof(Skylight));
            other.type = SkylightType::Null;
        }
        return *this;
    }

    __host__ Skylight& operator=(const Skylight&) = delete;

    // BUILD / TEARDOWN =======================================================

    __host__ Skylight* build() {
        Skylight* d_skylight_ptr;
        CUDAMemory::allocate<Skylight>(d_skylight_ptr);
        CUDAMemory::copy<Skylight>(d_skylight_ptr, this);
        return d_skylight_ptr;
    }

    __host__ void teardown() {
        switch (type) {
            case SkylightType::HDRI:
                if (cfg_hdri.data_ptr) CUDAMemory::free(cfg_hdri.data_ptr);
        }
    }

    // CONFIGURATION ==========================================================

    __host__ SimpleSkylightConfig& config_simple() { return cfg_simple; }
    __host__ HDRISkylightConfig&   config_hdri()   { return cfg_hdri;   }

    // UPDATING ===============================================================

    __host__ void load_hdri_file(const std::string& filepath) {
        if (type != SkylightType::HDRI) {
            throw std::runtime_error(
                "Skylight::update_hdri is only valid on skylights with type "
                "SkylightType::HDRI."
            );
        } 
        if (cfg_hdri.data_ptr) CUDAMemory::free(cfg_hdri.data_ptr);
        Sky::HDRI::load_file(filepath, cfg_hdri);
    }

    // SIMPLE =================================================================

    __host__ static Skylight simple(
        const Color start_color, 
        const Color end_color
    ) {
        SimpleSkylightConfig cfg;
        cfg.start = start_color;
        cfg.end = end_color;

        Skylight s(SkylightType::Simple); 
        s.cfg_simple = cfg;
        return s;
    }

    // HDRI ===================================================================

    __host__ static Skylight hdri(const std::string& filepath) {
        HDRISkylightConfig cfg;
        Skylight s(SkylightType::HDRI);
        s.cfg_hdri = cfg; s.load_hdri_file(filepath);
        return s;
    }

    __host__ static Skylight hdri() {
        HDRISkylightConfig cfg;
        Skylight s(SkylightType::HDRI);
        s.cfg_hdri = cfg;
        return s;
    }


    // SAMPLING ===============================================================

    __device__ Color sample(const Ray& ray) {
        switch (type) {
            case SkylightType::Null: return Color::black();
            case SkylightType::Simple: 
                return Sky::Simple::sample(ray, cfg_simple);
            case SkylightType::HDRI:
                return Sky::HDRI::sample(ray, cfg_hdri);
        }
        return Color::black();
    }

};


