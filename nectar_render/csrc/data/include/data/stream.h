#pragma once

#include "data/include/data/data_object.h"

class TransferStream {
public:

    /* CONSTRUCTORS */

    __host__ ~TransferStream() { destroy(); }

    __host__ TransferStream() : data(nullptr), C(0), H(0), W(0) { }

    /* LINKING */

    __host__ void link(DataObject* obj) {
        if (enabled) { destroy(); enabled = false; }
        C = obj->C; H = obj->H; W = obj->W;
        data = obj;
    }

    __host__ void unlink(DataObject* obj) { link(nullptr); }
    __host__ bool is_linked() const { return data != nullptr; }

    /* STREAM CONTROL */

    __host__ void start() {
        if (enabled) destroy();
        cudaMallocHost(&stream_buffer, n_bytes());
        cudaMalloc(&image_buffer, n_bytes());
        cudaStreamCreate(&transfer_stream);
        enabled = true;
    }

    __host__ void destroy() {
        if (!enabled) return;
        destroy_buffer(stream_buffer);
        destroy_buffer(image_buffer);
        if (transfer_stream) cudaStreamDestroy(transfer_stream);
        enabled = false;
    }

    /* DATA ACCESS */

    __host__ uintptr_t buffer_ptr() {
        return reinterpret_cast<uintptr_t>(stream_buffer);
    }

    __host__ uintptr_t readback() {
        to_image(data->view(), image_buffer);

        cudaMemcpyAsync(
            stream_buffer, image_buffer, n_bytes(),
            cudaMemcpyDeviceToHost, transfer_stream
        );

        cudaStreamSynchronize(transfer_stream);
        return buffer_ptr();
    }

    __host__ std::array<size_t, 3> shape() { return { C, H, W }; }
    __host__ size_t n_pixels()   const { return data->n_pixels(); }
    __host__ size_t n_elements() const { return data->n_elements(); }
    __host__ size_t n_bytes()    const { 
        return n_elements() * sizeof(uint8_t); 
    }

private:

    size_t C, H, W;
    bool enabled = false;
    DataObject* data = nullptr;

    uint8_t* stream_buffer = nullptr;
    uint8_t* image_buffer  = nullptr;
    cudaStream_t transfer_stream;

    __host__ bool destroy_buffer(uint8_t* buffer) {
        if (!buffer) return false;
        cudaFreeHost(buffer); buffer = nullptr;
        return true;
    }

};

