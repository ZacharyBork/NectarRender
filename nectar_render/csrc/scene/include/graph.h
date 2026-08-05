#pragma once

#include <cuda_runtime.h>

#include "light/include/skylight.h"
#include "hittable/include/bvh/node.h"
#include "hittable/include/hittable/hittable.h"

struct SceneGraph {
    BVHNode*   __restrict__ bvh_nodes;
    Hittable** __restrict__ objects;
    Hittable** __restrict__ lights;
    Material** __restrict__ materials;
    Skylight*  __restrict__ skylight;
    
    __device__ bool hit(
        const Ray&  ray,
        Interval    ray_t,
        HitRecord&  rec
    ) {
        uint32_t stack[STACK_SIZE];
        uint32_t stack_ptr = 0u;
        stack[stack_ptr++] = 0u;

        bool hit_anything = false;

        while (stack_ptr > 0) {
            uint32_t idx = stack[--stack_ptr];
            const BVHNode& node = bvh_nodes[idx];

            if (!node.bbox.hit(ray, ray_t)) continue;

            if (node.object != -1) {
                HitRecord tmp_rec;
                Hittable* current = objects[node.object];

                if (current->hit_test(ray, tmp_rec)) {
                    if (ray_t.surrounds(tmp_rec.t)) {
                        hit_anything = true;

                        rec = tmp_rec;
                        rec.hit_object     = current;
                        rec.object_id      = current->get_object_id();
                        rec.material_index = current->get_material_index();
                        rec.mat = materials[current->get_material_index()];
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

    static constexpr uint8_t STACK_SIZE = 64u;

};


