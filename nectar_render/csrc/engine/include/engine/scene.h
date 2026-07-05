#pragma once

#include <vector>
#include <optional>
#include <unordered_map>
#include <cuda_runtime.h>

#include "hittable/include/hittable/hittable.h"
#include "engine/include/engine/light.h"
#include "hittable/include/bvh/node.h"

struct SceneGraph {
    BVHNode*   bvh_nodes;
    Hittable** objects;
    Light**    lights;
    
    SkyLight skylight;

    size_t n_objects, n_nodes, n_lights;

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
        std::vector<Light*>&    lights,
        SkyLight& skylight
    ) {
        for (Light* light : lights) hittables.push_back(light);

        SceneGraph tmp;
        tmp.skylight  = skylight;
        tmp.n_objects = hittables.size();
        tmp.n_lights  = lights.size();

        build_bvh(tmp, hittables);
        build_device_hittables(tmp, hittables);
        build_device_lights(tmp, lights);

        size_t n_bytes = sizeof(tmp);
        cudaMalloc(&graph, n_bytes);
        cudaMemcpy(graph, &tmp, n_bytes, cudaMemcpyHostToDevice);

        host_to_device.clear();
    }

private:

    std::unordered_map<Hittable*, Hittable*> host_to_device;

    __host__ void build_bvh(
        SceneGraph& tmp, 
        std::vector<Hittable*>& hittables
    ) {
        BVH bvh;
        bvh.build(hittables);
        tmp.n_nodes = bvh.nodes.size();

        cudaMalloc(&tmp.bvh_nodes, tmp.n_nodes * sizeof(BVHNode));
        cudaMemcpy(tmp.bvh_nodes, bvh.nodes.data(),
                tmp.n_nodes * sizeof(BVHNode), cudaMemcpyHostToDevice);
    }

    __host__ void build_device_hittables(
        SceneGraph& tmp, 
        std::vector<Hittable*>& hittables
    ) {
        std::vector<Hittable*> d_obj_ptrs(tmp.n_objects);
        for (size_t i = 0; i < hittables.size(); i++) {
            Hittable* d_ptr = hittables[i]->build();
            d_obj_ptrs[i] = d_ptr;
            host_to_device[hittables[i]] = d_ptr;
        }

        cudaMalloc(&tmp.objects, tmp.n_objects * sizeof(Hittable*));
        cudaMemcpy(tmp.objects, d_obj_ptrs.data(),
                tmp.n_objects * sizeof(Hittable*), cudaMemcpyHostToDevice);
    }

    __host__ void build_device_lights(
        SceneGraph& tmp, 
        std::vector<Light*>& lights
    ) {
        std::vector<Hittable*> d_light_ptrs(tmp.n_lights);
        for (size_t i = 0; i < lights.size(); i++) {
            auto it = host_to_device.find(static_cast<Hittable*>(lights[i]));
            if (it == host_to_device.end())
                throw std::runtime_error(
                    "Light not found among traversable hittables"
                );
            d_light_ptrs[i] = it->second;
        }

        cudaMalloc(&tmp.lights, tmp.n_lights * sizeof(Hittable*));
        cudaMemcpy(tmp.lights, d_light_ptrs.data(),
                tmp.n_lights * sizeof(Hittable*), cudaMemcpyHostToDevice);
    }

};

