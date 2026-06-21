#pragma once

#include <cublas_v2.h>

cublasHandle_t get_cublas_handle();
void destroy_cublas_handle();
