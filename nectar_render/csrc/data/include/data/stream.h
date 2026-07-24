#pragma once

#include <atomic>

#include "core/include/core.h"
#include "data_object.h"

enum class StreamState { ACTIVE, INACTIVE };
enum class TonemapMethod{ REINHARD, REINHARD_EXTENDED, ACES };
struct StreamConfig {
    bool linear_to_gamma = true;

    bool apply_tonemapping = true;
    TonemapMethod tm_method = TonemapMethod::REINHARD;
    float tm_white_point = 1.0f;
    float tm_alpha = 1.0f;
};

void process_stream(DataView data, uint8_t* result, StreamConfig cfg);
void composite_overlay(
    uint8_t* data, uint8_t* mask, Color color, size_t C, size_t H, size_t W
);


class TransferStream {
public:

    size_t C, H, W;

    /* CONSTRUCTORS */

    ~TransferStream() { destroy(); }

    TransferStream() : data(nullptr), C(0), H(0), W(0) { }

    /* STREAM STATE */

    StreamState get_state() const {
        return state.load(std::memory_order_relaxed);
    }

    bool is_active()   const { return get_state()==StreamState::ACTIVE;   }
    bool is_inactive() const { return get_state()==StreamState::INACTIVE; }

    /* LINKING */

    void link(DataObject* obj) {
        if (is_active()) { destroy(); }
        C = obj->C; H = obj->H; W = obj->W;
        data = obj;
    }

    void unlink(DataObject* obj) { link(nullptr); }
    bool is_linked() const { return data != nullptr; }

    /* STREAM CONTROL */

    void start() {
        if (is_active()) destroy();
        cudaMallocHost(&stream_buffer, n_bytes());
        cudaMalloc(&image_buffer, n_bytes());
        cudaStreamCreate(&transfer_stream);
        set_state(StreamState::ACTIVE);
    }

    void destroy() {
        if (is_inactive()) return;
        destroy_buffer(stream_buffer);
        destroy_buffer(image_buffer);
        if (transfer_stream) cudaStreamDestroy(transfer_stream);
        set_state(StreamState::INACTIVE);
    }

    void update_config(StreamConfig cfg) {
        stream_config.store(cfg, std::memory_order_relaxed);
    }

    /* OVERLAYS */

    bool has_overlay() const { return overlay_mask != nullptr; }

    void remove_overlay() { 
        should_disable_overlay.store(true, std::memory_order_relaxed);
    }

    void overlay(uint8_t* mask_ptr, Color color = Color::white()) { 
        if (has_overlay()) { 
            cudaFree(overlay_mask); overlay_mask = nullptr; 
        }
        overlay_mask = mask_ptr; 
        overlay_color = color;
    }
    
    /* DATA ACCESS */

    uintptr_t buffer_ptr() {
        return reinterpret_cast<uintptr_t>(stream_buffer);
    }

    uintptr_t readback() {
        process_stream(data->view(), image_buffer, stream_config);
        
        
        if (overlay_mask != nullptr) {
            if (should_disable_overlay.load(std::memory_order_relaxed)) {
                cudaDeviceSynchronize();
                cudaFree(overlay_mask); 
                overlay_mask = nullptr;
                should_disable_overlay.store(false, std::memory_order_relaxed);
            } else {
                composite_overlay(
                    image_buffer, overlay_mask, overlay_color, C, H, W
                );
            }
        }

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

private:

    std::atomic<bool> should_disable_overlay { false };
    std::atomic<StreamState> state { StreamState::INACTIVE };

    
    std::atomic<StreamConfig> stream_config {};

    bool enabled = false;
    DataObject* data = nullptr;

    uint8_t* stream_buffer = nullptr;
    uint8_t* image_buffer  = nullptr;
    cudaStream_t transfer_stream;

    Color overlay_color;
    uint8_t* overlay_mask = nullptr;

    void set_state(StreamState new_state) {
        state.store(new_state, std::memory_order_relaxed);
    }

    bool destroy_buffer(uint8_t* buffer) {
        if (!buffer) return false;
        cudaFreeHost(buffer); buffer = nullptr;
        return true;
    }

};

