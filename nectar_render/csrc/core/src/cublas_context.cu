#include "core/include/core/cublas_context.h"

static cublasHandle_t cublas_handle = nullptr;

cublasHandle_t get_cublas_handle() {
    if (cublas_handle == nullptr)
        cublasCreate(&cublas_handle);
    return cublas_handle;
}

void destroy_cublas_handle() {
    if (cublas_handle != nullptr) {
        cublasDestroy(cublas_handle);
        cublas_handle = nullptr;
    }
}

