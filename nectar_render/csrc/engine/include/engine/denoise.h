#pragma once

#include "core/include/core.h"
#include "data/include/data.h"

void tv_denoise(
    DataObject& data,    
    const float lambda,
    const uint32_t iterations
);

class Denoiser { 
public: 

    __host__ Denoiser() { } 
    __host__ virtual ~Denoiser() { } 

    __host__ virtual void run(DataObject& data) const = 0;

};

class TVDenoiser : public Denoiser {
public:

    __host__ TVDenoiser(const float weight, const uint32_t iterations)
     : weight(weight), iterations(iterations) { }

    __host__ void run(DataObject& data) const override {
        tv_denoise(data, weight, iterations);
    }

private:

    float weight;
    uint32_t iterations;
};




