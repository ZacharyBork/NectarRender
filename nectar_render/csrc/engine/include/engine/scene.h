#pragma once

#include <vector>
#include <optional>

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
        int n = hittables.size();

        BVH bvh;
        bvh.build(hittables);

        std::vector<Hittable*> device_obj_ptrs(n);
        for (int i = 0; i < n; i++)
            device_obj_ptrs[i] = hittables[i]->build();

        cudaMalloc(&objects, n * sizeof(Hittable*));
        cudaMemcpy(
            objects, device_obj_ptrs.data(),
            n * sizeof(Hittable*), 
            cudaMemcpyHostToDevice
        );

        int n_nodes = bvh.nodes.size();
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
                if (objects[node.object]->hit(ray, ray_t, rec)) {
                    hit_anything = true;
                    ray_t.max = rec.t;
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

};

