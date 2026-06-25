#pragma once

#include "hittable/include/hittable/hittable.h"
#include "hittable/include/bvh/node.h"

struct Scene {
    BVHNode*    bvh_nodes;
    Hittable**  objects;
    int         n_nodes;
    int         n_objects;

    __device__ bool hit(
        const Ray&  ray,
        Interval    ray_t,
        HitRecord&  rec
    ) const {
        int stack[64];
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
};

