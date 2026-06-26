#pragma once

#include "core/include/core/vector.h"
#include "core/include/core/random.h"

class Perlin {
public:

    __host__ Perlin() : Perlin(42u) { }
    __host__ Perlin(unsigned int seed) : seed_(seed) { }

    __device__ Perlin(Vector3* rv, int* px, int* py, int* pz) 
        : randvec(rv), perm_x(px), perm_y(py), perm_z(pz) { }

    __host__ Perlin* build() {
        generate_random_floats();
        generate_permutation(&perm_x);
        generate_permutation(&perm_y);
        generate_permutation(&perm_z);
        return device_build<Perlin>(randvec, perm_x, perm_y, perm_z);
    }

    __device__ float noise(const Vector3& p) const {
        float u = p.x() - floorf(p.x());
        float v = p.y() - floorf(p.y());
        float w = p.z() - floorf(p.z());

        int i = int(floorf(p.x()));
        int j = int(floorf(p.y()));
        int k = int(floorf(p.z()));
        Vector3 c[2][2][2];

        for (int di=0; di < 2; di++)
            for (int dj=0; dj < 2; dj++)
                for (int dk=0; dk < 2; dk++)
                    c[di][dj][dk] = randvec[
                        perm_x[(i+di) & 255] ^
                        perm_y[(j+dj) & 255] ^
                        perm_z[(k+dk) & 255]
                    ];

        return perlin_interp(c, u, v, w);
    }

    __device__ float turb(const Vector3& p, int iterations) const {
        float   accum  = 0.0;
        float   weight = 1.0;
        Vector3 temp_p = p;

        for (int i = 0; i < iterations; i++) {
            accum  += weight * noise(temp_p);
            weight *= 0.5;
            temp_p *= 2;
        }

        return fabsf(accum);
    }

private:

    unsigned int seed_ = 0u;
    static const int point_count = 256;
    Vector3* randvec;
    int* perm_x;
    int* perm_y;
    int* perm_z;

    __host__ unsigned int current_seed() { 
        seed_++;
        return seed_;
    }

    __host__ void generate_random_floats() {
        Vector3 v[point_count];

        for (int i = 0; i < point_count; i++)
            v[i] = Vector3(
                random_float(0.0f, 1.0f, current_seed()),
                random_float(0.0f, 1.0f, current_seed()),
                random_float(0.0f, 1.0f, current_seed())
            );

        size_t bytes = point_count * sizeof(Vector3);
        cudaMalloc(&randvec, bytes);
        cudaMemcpy(randvec, v, bytes, cudaMemcpyHostToDevice);
    }

    __host__ void generate_permutation(int** p) {
        int v[point_count];
        for (int i = 0; i < point_count; i++) v[i] = i;
        for (int i = point_count-1; i > 0; i--) {
            int target = random_int(0, i, current_seed());
            std::swap(v[i], v[target]);
        }
        size_t bytes = point_count * sizeof(int);
        cudaMalloc(p, bytes);
        cudaMemcpy(*p, v, bytes, cudaMemcpyHostToDevice);
    }

    __device__ static float perlin_interp(
        const Vector3 c[2][2][2], 
        float u, 
        float v, 
        float w
    ) {
        auto uu = u*u*(3-2*u);
        auto vv = v*v*(3-2*v);
        auto ww = w*w*(3-2*w);
        auto accum = 0.0;

        for (int i=0; i < 2; i++)
            for (int j=0; j < 2; j++)
                for (int k=0; k < 2; k++) {
                    Vector3 weight_v(u-i, v-j, w-k);
                    accum += (i*uu + (1-i)*(1-uu))
                           * (j*vv + (1-j)*(1-vv))
                           * (k*ww + (1-k)*(1-ww))
                           * dot(c[i][j][k], weight_v);
                }

        return accum;
    }

};


