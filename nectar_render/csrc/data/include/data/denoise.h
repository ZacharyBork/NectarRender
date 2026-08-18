#pragma once

#include "core/include/core.h"
#include "data_object.h"

#include <OpenImageDenoise/oidn.h>

// ============================================================================
// INTERLEAVING
// ============================================================================

void planar_to_interleaved(DataView data, float* out, cudaStream_t stream);
void interleaved_to_planar(float* in, DataView data, cudaStream_t stream);

// ============================================================================
// OPEN IMAGE DENOISE
// ============================================================================

enum class DenoiseFilterQuality{ DEFAULT, HIGH, BALANCED, FAST };

class OIDNDenoiser {
public:
    ~OIDNDenoiser() { destroy(); }

    void setup(
        size_t H, size_t W,
        cudaStream_t stream,
        bool clean_auxiliaries = true,
        OIDNQuality quality = OIDN_QUALITY_DEFAULT,
        float input_scale = 1.0f
    ) {
        destroy();
        this->W = W; this->H = H;

        int device_id = 0;
        device = oidnNewCUDADevice(&device_id, &stream, 1);
        oidnCommitDevice(device);

        CUDAMemory::allocate<float>(color_buf,  W * H * 3);
        CUDAMemory::allocate<float>(albedo_buf, W * H * 3);
        CUDAMemory::allocate<float>(normal_buf, W * H * 3);
        CUDAMemory::allocate<float>(output_buf, W * H * 3);

        filter = oidnNewFilter(device, "RT");
        oidnSetSharedFilterImage(
            filter, "color",  color_buf,  OIDN_FORMAT_FLOAT3, W, H, 0, 0, 0
        );
        oidnSetSharedFilterImage(
            filter, "albedo", albedo_buf, OIDN_FORMAT_FLOAT3, W, H, 0, 0, 0
        );
        oidnSetSharedFilterImage(
            filter, "normal", normal_buf, OIDN_FORMAT_FLOAT3, W, H, 0, 0, 0
        );
        oidnSetSharedFilterImage(
            filter, "output", output_buf, OIDN_FORMAT_FLOAT3, W, H, 0, 0, 0
        );
        
        oidnSetFilterBool(filter, "hdr", true);
        oidnSetFilterBool(filter, "cleanAux", clean_auxiliaries);
        oidnSetFilterInt(filter, "quality", quality);
        oidnSetFilterFloat(filter, "inputScale", input_scale);
        oidnCommitFilter(filter);
    }

    void denoise(
        DataView beauty, 
        DataView albedo, 
        DataView normal,
        DataView out,
        cudaStream_t stream
    ) {
        planar_to_interleaved(beauty, color_buf,  stream);
        planar_to_interleaved(albedo, albedo_buf, stream);
        planar_to_interleaved(normal, normal_buf, stream);

        oidnExecuteFilterAsync(filter);

        interleaved_to_planar(output_buf, out, stream);

        const char* err = nullptr;
        if (oidnGetDeviceError(device, &err) != OIDN_ERROR_NONE)
            std::cerr << "OIDN error: " << err << std::endl;
    }

    void destroy() {
        if (filter)     { oidnReleaseFilter(filter);    filter     = nullptr; }
        if (device)     { oidnReleaseDevice(device);    device     = nullptr; }
        if (color_buf)  { CUDAMemory::free(color_buf);  color_buf  = nullptr; }
        if (albedo_buf) { CUDAMemory::free(albedo_buf); albedo_buf = nullptr; }
        if (normal_buf) { CUDAMemory::free(normal_buf); normal_buf = nullptr; }
        if (output_buf) { CUDAMemory::free(output_buf); output_buf = nullptr; }
    }

private:

    size_t W = 0, H = 0;
    OIDNDevice device = nullptr;
    OIDNFilter filter = nullptr;
    float *color_buf  = nullptr, 
          *albedo_buf = nullptr, 
          *normal_buf = nullptr, 
          *output_buf = nullptr;

};


