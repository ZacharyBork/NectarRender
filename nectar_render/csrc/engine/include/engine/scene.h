#pragma once

#include <vector>
#include <optional>
#include <cuda_runtime.h>

#include "engine/include/engine/light.h"
#include "hittable/include/hittable/hittable.h"
#include "hittable/include/bvh/node.h"

struct SceneGraph {
    BVHNode*   bvh_nodes;
    Hittable** objects;
    
    SkyLight skylight;

    size_t n_objects;
    size_t n_nodes;

    __device__ bool hit(
        const Ray&  ray,
        Interval    ray_t,
        HitRecord&  rec
    ) const {
        uint8_t stack[STACK_SIZE];
        uint8_t stack_ptr  = 0u;
        stack[stack_ptr++] = 0u;

        bool hit_anything = false;

        while (stack_ptr > 0) {
            uint8_t idx = stack[--stack_ptr];
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

    static constexpr uint8_t STACK_SIZE = 64u;

};

class Scene {
public:

    SceneGraph* graph;

    __host__ Scene(
        std::vector<Hittable*>& hittables,
        SkyLight& skylight
    ) {
        SceneGraph tmp;

        tmp.skylight  = skylight;
        tmp.n_objects = hittables.size();

        BVH bvh;
        bvh.build(hittables);

        std::vector<Hittable*> device_obj_ptrs(tmp.n_objects);
        for (int i = 0; i < tmp.n_objects; i++)
            device_obj_ptrs[i] = hittables[i]->build();

        cudaMalloc(&tmp.objects, tmp.n_objects * sizeof(Hittable*));
        cudaMemcpy(
            tmp.objects, device_obj_ptrs.data(),
            tmp.n_objects * sizeof(Hittable*), 
            cudaMemcpyHostToDevice
        );

        tmp.n_nodes = bvh.nodes.size();
        cudaMalloc(&tmp.bvh_nodes, tmp.n_nodes * sizeof(BVHNode));
        cudaMemcpy(
            tmp.bvh_nodes, bvh.nodes.data(),
            tmp.n_nodes * sizeof(BVHNode), 
            cudaMemcpyHostToDevice
        );

        size_t n_bytes = sizeof(tmp);
        cudaMalloc(&graph, n_bytes);
        cudaMemcpy(graph, &tmp, n_bytes, cudaMemcpyHostToDevice);
    }

};

