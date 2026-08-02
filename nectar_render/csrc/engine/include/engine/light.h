#pragma once

#include "core/include/core.h"
#include "engine/include/engine/ray.h"
#include "material/include/material/texture.h"

// ============================================================================
// HDRI
// ============================================================================

struct HDRIView {

    float* data_ptr;
    int C, H, W;
    float intensity;

    __device__ Color sample(const Vector3 ray_dir) {
        Vector2 uv = hdri_uvs(ray_dir);
        return sample_hdri(uv) * intensity;
    }

    __device__ Vector2 hdri_uvs(const Vector3& dir) {
        float u = atan2f(dir.z(), dir.x()) / PI2 + 0.5f;
        float v = acosf(fminf(fmaxf(dir.y(), -1.0f), 1.0f)) / PI;
        return Vector2(u, v);
    }

    __device__ Color sample_hdri(Vector2 uv) {
        float fx = uv.u() * (float)W - 0.5f;
        float fy = uv.v() * (float)H - 0.5f;

        int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
        int x1 = x0 + 1, y1 = y0 + 1;
        float tx = fx - (float)x0, ty = fy - (float)y0;

        x0 = ((x0 % (int)W) + (int)W) % (int)W;
        x1 = ((x1 % (int)W) + (int)W) % (int)W;
        y0 = fmaxf(0, fminf((int)H - 1, y0));
        y1 = fmaxf(0, fminf((int)H - 1, y1));

        auto texel = [&](int x, int y) {
            size_t hw = H * W;
            size_t idx = (size_t)y * W + (size_t)x;
            return Color(
                data_ptr[0*hw+idx], data_ptr[1*hw+idx], data_ptr[2*hw+idx]
            );
        };

        Color c00 = texel(x0, y0), c10 = texel(x1, y0);
        Color c01 = texel(x0, y1), c11 = texel(x1, y1);

        Color top    = c00 * (1.0f - tx) + c10 * tx;
        Color bottom = c01 * (1.0f - tx) + c11 * tx;
        return top * (1.0f - ty) + bottom * ty;
    }
};

struct HDRI {

    float* data_ptr = nullptr;
    int C = 0, H = 0, W = 0;
    float intensity = 1.0f;

    __host__ ~HDRI() { 
        if (data_ptr) { CUDAMemory::free(data_ptr); data_ptr = nullptr; } 
    }

    __host__ HDRI() = default;
    __host__ HDRI(const HDRI&) = delete;
    __host__ HDRI& operator=(const HDRI&) = delete;

    __host__ HDRI(const std::string& filepath, float intensity_ = 1.0f)
        : intensity(intensity_)
    {
        std::vector<float> image_data = load_image_float(filepath, C, H, W);
        size_t n_elements = (size_t)(C * H * W);
        CUDAMemory::allocate<float>(data_ptr, n_elements);
        CUDAMemory::copy<float>(data_ptr, image_data.data(), n_elements);
    }

    __host__ HDRIView view() const {
        return HDRIView{ data_ptr, C, H, W, intensity };
    }

    
};

// ============================================================================
// SKYLIGHT
// ============================================================================

struct SkyLightView {

    Color start, end;
    bool is_hdr;
    HDRIView hdri;

    __device__ Color sample(const Ray& ray) {
        Vector3 dir = normalize(ray.direction());
        if (is_hdr) return hdri.sample(dir);

        float a = 0.5 * (dir.y() + 1.0);
        return (1.0 - a) * start + a * end;
    }
    
};

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

    __host__ static SkyLight hdri(
        std::string filepath, 
        float intensity = 1.0f
    ) {
        SkyLight s;
        s.is_hdr = true;
        s.hdri_instance = std::make_shared<HDRI>(filepath, intensity);
        return s;
    }

    __host__ SkyLightView view() {
        
        return SkyLightView{ start, end, is_hdr, hdri_instance->view() };
    }

private:

    Color start, end;
    bool is_hdr = false;
    std::shared_ptr<HDRI> hdri_instance;


    
};


