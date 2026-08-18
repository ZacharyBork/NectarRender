#pragma once

#include <iostream>
#include <stdint.h>

#include "random/include/hash.h"

class MonteCarlo {
public:

    uint32_t n_iterations;

    __host__ MonteCarlo(const uint32_t n_iterations = 100000u) 
        : n_iterations(n_iterations) { }

    __host__ __device__ void estimate_pi() {
        uint32_t inside_circle = 0u;
        uint32_t runs = 0u;


        uint32_t inside_circle_stratified = 0;
        uint32_t sqrt_N = 1000;

        for (uint32_t i = 0; i < sqrt_N; i++) {
            for (uint32_t j = 0; j < sqrt_N; j++) {
                float x = pcg_float_in_range(i+j, -1.0f, 1.0f);
                float y = pcg_float_in_range(i*i+j*j, -1.0f, 1.0f);
                if (x*x + y*y < 1)
                    inside_circle++;

                x = 2*((i + pcg_float(sqrt_N + i+j)) / sqrt_N) - 1;
                y = 2*((j + pcg_float(sqrt_N + i*i+j*j)) / sqrt_N) - 1;
                if (x*x + y*y < 1)
                    inside_circle_stratified++;
            }
        }

        std::cout
            << "Regular    Estimate of Pi = "
            << (4.0 * inside_circle) / (sqrt_N*sqrt_N) << '\n'
            << "Stratified Estimate of Pi = "
            << (4.0 * inside_circle_stratified) / (sqrt_N*sqrt_N) << '\n';
    }

protected:



private:



};


