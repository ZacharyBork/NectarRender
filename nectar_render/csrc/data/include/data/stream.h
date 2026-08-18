#pragma once

#include <atomic>
#include <mutex>

#include "core/include/core.h"
#include "data_object.h"
#include "denoise.h"

enum class StreamState { ACTIVE, INACTIVE, FROZEN };
enum class TonemapMethod{ REINHARD, REINHARD_EXTENDED, ACES };

struct StreamConfig {
    bool apply_denoising = true;
    bool denoise_clean_auxiliaries = true;
    DenoiseFilterQuality denoise_quality = DenoiseFilterQuality::FAST;
    float denoise_input_scale = 1.0f;


    bool linear_to_gamma = true;

    bool  apply_white_balance = true;
    float wb_temperature = 6800.0f;
    float wb_tint = 1.0f;

    bool apply_tonemapping = true;
    TonemapMethod tm_method = TonemapMethod::REINHARD;
    float tm_white_point = 1.0f;
    float tm_alpha = 1.0f;
};

void process_stream(
    DataView data, 
    uint8_t* result, 
    StreamConfig cfg, 
    cudaStream_t stream
);
void composite_overlay(
    uint8_t* data, 
    uint8_t* mask, 
    Color color, 
    size_t C, size_t H, size_t W, 
    cudaStream_t stream
);


class TransferStream {
public:

    size_t C, H, W;

    /* CONSTRUCTORS */

    ~TransferStream() { destroy(); }

    TransferStream() : data(nullptr), C(0), H(0), W(0) { }

    /* STREAM STATE */

    StreamState get_state() const {
        return state.load(relaxed);
    }

    bool is_active()   const { return get_state()==StreamState::ACTIVE;   }
    bool is_inactive() const { return get_state()==StreamState::INACTIVE; }
    bool is_frozen()   const { return get_state()==StreamState::FROZEN;   }

    /* LINKING */

    void link(DataObject* obj) {
        C = obj->C; H = obj->H; W = obj->W;
        data = obj;
    }

    void unlink() { C = H = W = 0UL; data = nullptr; }
    bool is_linked() const { return data != nullptr; }

    /* STREAM CONTROL */

    void freeze() { 
        if (is_active()) {
            cudaStreamSynchronize(transfer_stream);
            set_state(StreamState::FROZEN); 
        }
        
    }
    void unfreeze() { if (is_frozen()) set_state(StreamState::ACTIVE); }

    void start() {
        if (!is_inactive()) destroy();

        CUDAMemory::allocate_host(stream_buffer, n_elements());
        CUDAMemory::allocate(image_buffer, n_bytes());
        cudaStreamCreateWithFlags(&transfer_stream, cudaStreamNonBlocking);
        
        set_state(StreamState::ACTIVE);
    }

    void destroy() {
        if (is_inactive()) return;

        if (stream_buffer)   CUDAMemory::free_host(stream_buffer);
        if (image_buffer)    CUDAMemory::free(image_buffer);
        if (transfer_stream) cudaStreamDestroy(transfer_stream);

        set_state(StreamState::INACTIVE);
    }

    StreamConfig get_config() { return stream_config.load(relaxed); }

    void update_config(StreamConfig cfg) {
        stream_config.store(cfg, relaxed);
    }

    template<typename Func>
    void freeze_for(Func&& fn) {
        std::lock_guard<std::mutex> lock(readback_mutex);
        if (is_active()) {
            cudaStreamSynchronize(transfer_stream);
            set_state(StreamState::FROZEN);
        }
        fn();
    }

    template<typename Func>
    void guarded(Func&& fn) {
        std::lock_guard<std::mutex> lock(readback_mutex);
        fn();
    }

    /* OVERLAYS */

    bool has_overlay() const { return overlay_mask != nullptr; }

    void remove_overlay() { 
        should_disable_overlay.store(true, relaxed);
    }

    void overlay(uint8_t* mask_ptr, Color color = Color::white()) { 
        std::lock_guard<std::mutex> lock(readback_mutex);
        if (has_overlay()) { 
            CUDAMemory::free(overlay_mask); overlay_mask = nullptr; 
        }
        overlay_mask = mask_ptr; 
        overlay_color = color;
    }
    
    /* DATA ACCESS */

    uintptr_t buffer_ptr() {
        return reinterpret_cast<uintptr_t>(stream_buffer);
    }

    uintptr_t readback() {
        std::lock_guard<std::mutex> lock(readback_mutex);
        if (is_frozen()) return buffer_ptr();

        DataView source = data->view();

        if (get_config().apply_denoising 
        &&  albedo_data && normal_data) {
            denoiser.denoise(
                data->view(), 
                albedo_data->view(), 
                normal_data->view(), 
                denoised_output.view(),
                transfer_stream
            );
            source = denoised_output.view();
        }

        process_stream(source, image_buffer, get_config(), transfer_stream);
        handle_overlay();

        cudaMemcpyAsync(
            stream_buffer, image_buffer, n_bytes(),
            cudaMemcpyDeviceToHost, transfer_stream
        );

        cudaStreamSynchronize(transfer_stream);
        return buffer_ptr();
    }

    std::array<size_t, 3> shape() { return { C, H, W }; }
    size_t n_pixels()   const { return data->n_pixels(); }
    size_t n_elements() const { return data->n_elements(); }
    size_t n_bytes()    const { 
        return n_elements() * sizeof(uint8_t); 
    }

    /* DENOISING */

    void link_denoise_aux(DataObject* albedo, DataObject* normal) {
        albedo_data = albedo;
        normal_data = normal;
        denoised_output = DataObject(3, H, W);
        rebuild_denoiser();
    }

    void enable_denoising() { 
        StreamConfig cfg = get_config();
        if (cfg.apply_denoising) return;
        cfg.apply_denoising = true;
        stream_config.store(cfg, relaxed);
    }
    void disable_denoising() {
        StreamConfig cfg = get_config();
        if (!cfg.apply_denoising) return;
        cfg.apply_denoising = false;
        stream_config.store(cfg, relaxed);
    }

    void rebuild_denoiser() {
        OIDNQuality q = OIDN_QUALITY_DEFAULT;
        StreamConfig cfg = get_config();
        switch (cfg.denoise_quality) {
            case DenoiseFilterQuality::DEFAULT: 
                q = OIDN_QUALITY_DEFAULT; break;
            case DenoiseFilterQuality::HIGH: 
                q = OIDN_QUALITY_HIGH; break;
            case DenoiseFilterQuality::BALANCED: 
                q = OIDN_QUALITY_BALANCED; break;
            case DenoiseFilterQuality::FAST: 
                q = OIDN_QUALITY_FAST; break;
        }

        {
            std::lock_guard<std::mutex> lock(readback_mutex);
            denoiser.destroy();
            denoiser.setup(
                H, W, transfer_stream, cfg.denoise_clean_auxiliaries, 
                q, cfg.denoise_input_scale
            );
        }
    }

private:

    std::mutex readback_mutex;

    std::atomic<bool> should_disable_overlay { false };
    std::atomic<StreamState>  state { StreamState::INACTIVE };
    std::atomic<StreamConfig> stream_config {};

    bool enabled = false;
    DataObject* data = nullptr;

    uint8_t* stream_buffer = nullptr;
    uint8_t* image_buffer  = nullptr;
    cudaStream_t transfer_stream;

    DataObject* albedo_data = nullptr;
    DataObject* normal_data = nullptr;
    OIDNDenoiser denoiser;
    DataObject denoised_output;

    Color overlay_color;
    uint8_t* overlay_mask = nullptr;

    void set_state(StreamState new_state) {
        state.store(new_state, relaxed);
    }

    void handle_overlay() {
        if (!overlay_mask) return;
        bool disable = should_disable_overlay.exchange(
            false, relaxed
        );
        if (disable) { 
            CUDAMemory::free(overlay_mask); 
            overlay_mask = nullptr;
        } 
        else composite_overlay(
            image_buffer, overlay_mask, overlay_color, 
            C, H, W, transfer_stream
        );
    }

};


