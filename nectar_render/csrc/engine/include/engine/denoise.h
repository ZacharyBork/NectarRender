#pragma once

#include "core/include/core.h"
#include "engine/include/engine/data.h"

void tv_denoise(
    DataObject& data,    
    const float lambda,
    const unsigned int iterations
);

class Denoiser { 
public: 

    __host__ Denoiser() { } 
    __host__ virtual ~Denoiser() { } 

    __host__ virtual void run(DataObject& data) const = 0;

};

class TVDenoiser : public Denoiser {
public:

    __host__ TVDenoiser(const float weight, const unsigned int iterations)
     : weight(weight), iterations(iterations) { }

    __host__ void run(DataObject& data) const override {
        tv_denoise(data, weight, iterations);
    }

private:

    float weight;
    unsigned int iterations;
};




