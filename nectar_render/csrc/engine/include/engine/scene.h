#pragma once

#include <vector>
#include <optional>
#include <cuda_runtime.h>

#include "engine/include/engine/light.h"
#include "hittable/include/hittable/hittable.h"
#include "hittable/include/bvh/node.h"

class Scene {
public:

    SkyLight skylight;

    __host__ Scene(
        std::vector<Hittable*>& hittables,
        SkyLight& skylight
    ) : skylight(skylight) { build(hittables); }

    __host__ void build(std::vector<Hittable*>& hittables) {
        n_objects = hittables.size();

        BVH bvh;
        bvh.build(hittables);

        std::vector<Hittable*> device_obj_ptrs(n_objects);
        for (int i = 0; i < n_objects; i++)
            device_obj_ptrs[i] = hittables[i]->build();

        cudaMalloc(&objects, n_objects * sizeof(Hittable*));
        cudaMemcpy(
            objects, device_obj_ptrs.data(),
            n_objects * sizeof(Hittable*), 
            cudaMemcpyHostToDevice
        );

        n_nodes = bvh.nodes.size();
        cudaMalloc(&bvh_nodes, n_nodes * sizeof(BVHNode));
        cudaMemcpy(
            bvh_nodes, bvh.nodes.data(),
            n_nodes * sizeof(BVHNode), 
            cudaMemcpyHostToDevice
        );
    }

    __device__ bool hit(
        const Ray&  ray,
        Interval    ray_t,
        HitRecord&  rec
    ) const {
        int stack[stack_size];
        int stack_ptr = 0;
        stack[stack_ptr++] = 0;

        bool hit_anything = false;

        while (stack_ptr > 0) {
            int idx = stack[--stack_ptr];
            const BVHNode& node = bvh_nodes[idx];

            if (!node.bbox.hit(ray, ray_t)) continue;

            if (node.object != -1) {
                HitRecord tmp_rec;
                
                if (objects[node.object]->hit_test(ray, tmp_rec)) {
                    if (ray_t.surrounds(tmp_rec.t)) {
                        rec = tmp_rec;
                        hit_anything = true;
                        ray_t.max = tmp_rec.t;
                    }
                }

            } else {
                stack[stack_ptr++] = node.left;
                stack[stack_ptr++] = node.right;
            }
        }
        return hit_anything;
    }

private:

    static constexpr int stack_size = 64;
    BVHNode*   bvh_nodes;
    Hittable** objects;
    uint32_t n_objects;
    uint32_t n_nodes;

};

