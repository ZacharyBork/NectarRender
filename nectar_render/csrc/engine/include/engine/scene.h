#pragma once

#include <vector>
#include <optional>
#include <unordered_map>
#include <cuda_runtime.h>

#include "hittable/include/bvh/node.h"
#include "hittable/include/hittable/hittable.h"
#include "hittable/include/hittable/registry.h"

#include "material/include/material/registry.h"
#include "engine/include/engine/light.h"

struct SceneGraph {
    BVHNode*   bvh_nodes;
    Hittable** objects;
    Light**    lights;
    Material** materials;
    
    SkyLight skylight;

    

    size_t n_objects, n_nodes, n_lights;

    __device__ bool hit(
        const Ray&  ray,
        Interval    ray_t,
        HitRecord&  rec
    ) {
        uint32_t stack[STACK_SIZE];
        uint32_t stack_ptr  = 0u;
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
                        rec.object_index = node.object;
                        rec.material_index = current->material_index;
                        rec.mat = materials[current->material_index];
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

    SceneGraph* graph = nullptr;
    SceneGraph h_graph{};

    std::vector<Hittable*> hittables;
    std::vector<Light*>    lights;
    SkyLight skylight;

    HittablesRegistry hittables_registry;
    MaterialRegisty   material_registry;

    __host__ Scene() 
      : hittables(std::vector<Hittable*>{}),
        lights(std::vector<Light*>{}),
        skylight(SkyLight())
    { }

    __host__ Scene(
        std::vector<Hittable*> hittables,
        std::vector<Light*>    lights,
        SkyLight&              skylight
    ) : hittables(std::move(hittables)),
        lights(std::move(lights)),
        skylight(skylight)
    { 
        for (Light* light : this->lights) this->hittables.push_back(light);
        build(); 
    }

    __host__ void teardown() {
        if (!graph) return;
        cudaFree(graph); 

        hittables_registry.destroy_device_hittables();
        material_registry.destroy_device_materials();

        cudaFree(h_graph.bvh_nodes);
        cudaFree(h_graph.lights);

        graph   = nullptr;
        h_graph = SceneGraph();
    }

    __host__ void build() {
        teardown();

        build_bvh();

        material_registry.register_materials(hittables);
        h_graph.materials = material_registry.device_materials();

        hittables_registry.register_hittables(hittables);
        h_graph.objects = hittables_registry.device_hittables();

        h_graph.skylight  = skylight;
        h_graph.n_objects = hittables.size();
        h_graph.n_lights  = lights.size();

        build_device_lights();

        size_t n_bytes = sizeof(h_graph);
        cudaMalloc(&graph, n_bytes);
        cudaMemcpy(graph, &h_graph, n_bytes, cudaMemcpyHostToDevice);

        host_to_device.clear();
    }

    __host__ Hittable* object_at_index(const uint32_t index) {
        if (index >= hittables.size())
            throw std::runtime_error(
                "Scene::object_at_index(): index " + 
                std::to_string(index) + " out of range"
            );
        return hittables[index];
    }

private:

    std::unordered_map<Hittable*, Hittable*> host_to_device;

    __host__ void build_bvh() {
        BVH bvh;
        bvh.build(hittables);
        h_graph.n_nodes = bvh.nodes.size();

        cudaMalloc(&h_graph.bvh_nodes, h_graph.n_nodes * sizeof(BVHNode));
        cudaMemcpy(
            h_graph.bvh_nodes, bvh.nodes.data(),
            h_graph.n_nodes * sizeof(BVHNode), cudaMemcpyHostToDevice
        );
    }

    __host__ void build_device_lights() {
        std::vector<Hittable*> d_light_ptrs(h_graph.n_lights);
        for (size_t i = 0; i < lights.size(); i++) {
            d_light_ptrs[i] = hittables_registry.host_to_device(
                static_cast<Hittable*>(lights[i])
            );
        }

        cudaMalloc(&h_graph.lights, h_graph.n_lights * sizeof(Hittable*));
        cudaMemcpy(
            h_graph.lights, d_light_ptrs.data(),
            h_graph.n_lights * sizeof(Hittable*), cudaMemcpyHostToDevice
        );
    }

};

