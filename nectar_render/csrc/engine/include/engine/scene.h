#pragma once

#include <vector>
#include <atomic>
#include <optional>
#include <unordered_map>
#include <cuda_runtime.h>

#include "hittable/include/bvh/node.h"
#include "hittable/include/hittable/hittable.h"
#include "hittable/include/hittable/registry.h"

#include "material/include/material/registry.h"
#include "engine/include/engine/skylight.h"

struct SceneGraph {
    BVHNode*   __restrict__ bvh_nodes;
    Hittable** __restrict__ objects;
    Hittable** __restrict__ lights;
    Material** __restrict__ materials;
    
    Skylight* skylight;

    size_t n_lights;

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
                        rec.object_index   = current->get_object_index();
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

class Scene {
public:

    SceneGraph* graph = nullptr;
    SceneGraph h_graph{};

    std::vector<Hittable*> lights;
    Skylight skylight;

    HittablesRegistry hittables_registry;
    MaterialRegisty   material_registry;

    // CONSTRUCTORS ===========================================================

    __host__ ~Scene() { teardown(); }

    __host__ Scene() 
      : lights(std::vector<Hittable*>{}),
        skylight(std::move(Skylight()))
    { }

    __host__ Scene(
        std::vector<Hittable*> hittables,
        std::vector<Hittable*> lights,
        Skylight               skylight
    ) : lights(std::move(lights)),
        skylight(std::move(skylight))
    { 
        for (Hittable* light : this->lights) hittables.push_back(light);
        hittables_registry = HittablesRegistry(hittables);
    }


    __host__ Scene(Scene&& other) noexcept
      : lights(std::move(other.lights)),
        skylight(std::move(other.skylight)),
        hittables_registry(std::move(other.hittables_registry)),
        material_registry(std::move(other.material_registry)),
        graph(other.graph),
        h_graph(other.h_graph)
    {
        materials_build_pending.store(
            other.materials_build_pending.load(std::memory_order_relaxed), 
            std::memory_order_relaxed
        );
        hittables_build_pending.store(
            other.hittables_build_pending.load(std::memory_order_relaxed), 
            std::memory_order_relaxed
        );
        skylight_update_pending.store(
            other.skylight_update_pending.load(std::memory_order_relaxed), 
            std::memory_order_relaxed
        );
        other.graph = nullptr;
    }

    __host__ Scene& operator=(Scene&& other) noexcept {
        if (this != &other) {
            lights = std::move(other.lights);
            skylight = std::move(other.skylight);
            hittables_registry = std::move(other.hittables_registry);
            material_registry = std::move(other.material_registry);
            graph = other.graph;
            h_graph = other.h_graph;
            materials_build_pending.store(
                other.materials_build_pending.load(std::memory_order_relaxed), 
                std::memory_order_relaxed
            );
            hittables_build_pending.store(
                other.hittables_build_pending.load(std::memory_order_relaxed), 
                std::memory_order_relaxed
            );
            skylight_update_pending.store(
                other.skylight_update_pending.load(std::memory_order_relaxed), 
                std::memory_order_relaxed
            );
            other.graph = nullptr;
        }
        return *this;
    }

    __host__ Scene(const Scene&) = delete;
    __host__ Scene& operator=(const Scene&) = delete;

    // BUILD / TEARDOWN =======================================================

    __host__ void teardown() {
        if (!graph) return;
        CUDAMemory::free(graph); 

        hittables_registry.destroy_device_hittables();
        material_registry.teardown();

        CUDAMemory::free(h_graph.lights);

        graph   = nullptr;
        h_graph = SceneGraph();
    }

    __host__ void build() {
        material_registry.register_materials(hittables_registry.objects());
        material_registry.build_device_materials();
        h_graph.materials = material_registry.device_materials();

        hittables_registry.build();
        h_graph.objects = hittables_registry.device_hittables();
        h_graph.bvh_nodes = hittables_registry.bvh_nodes;

        h_graph.skylight = skylight.build();
        h_graph.n_lights = lights.size();

        build_device_lights();

        CUDAMemory::allocate<SceneGraph>(graph);
        CUDAMemory::copy<SceneGraph>(graph, &h_graph);
    }

    // REQUESTS ===============================================================

    __host__ void request_reset(
        const bool rebuild_hittables = false,
        const bool rebuild_materials = false,
        const bool update_skylight   = false
    ) {
        if (rebuild_hittables)
            hittables_build_pending.store(true, std::memory_order_relaxed);
        if (rebuild_materials)
            materials_build_pending.store(true, std::memory_order_relaxed);
        if (update_skylight)
            skylight_update_pending.store(true, std::memory_order_relaxed);
    }

    __host__ bool is_pending_update() {
        return materials_build_pending.load(std::memory_order_relaxed)
            || hittables_build_pending.load(std::memory_order_relaxed)
            || skylight_update_pending.load(std::memory_order_relaxed);
    }

    // UPDATING ===============================================================

    __host__ void rebuild_hittables_registry() {
        hittables_registry.destroy_device_hittables();

        hittables_registry.build();
        h_graph.objects = hittables_registry.device_hittables();
        h_graph.bvh_nodes = hittables_registry.bvh_nodes;

        build_device_lights();
    }

    __host__ void rebuild_materials_registry() {
        material_registry.destroy_device_materials();
        material_registry.build_device_materials();
        h_graph.materials = material_registry.device_materials();
    }

    __host__ void update() {
        if (materials_build_pending.exchange(false, std::memory_order_relaxed))
            rebuild_materials_registry();

        if (hittables_build_pending.exchange(false, std::memory_order_relaxed))
            rebuild_hittables_registry();

        if (skylight_update_pending.exchange(false, std::memory_order_relaxed))
            h_graph.skylight = skylight.build();

        if (!graph) CUDAMemory::allocate<SceneGraph>(graph);
        CUDAMemory::copy<SceneGraph>(graph, &h_graph);
    }

private:

    std::atomic<bool> materials_build_pending { false };
    std::atomic<bool> hittables_build_pending { false };
    std::atomic<bool> skylight_update_pending { false };

    __host__ void build_device_lights() {
        if (h_graph.lights) CUDAMemory::free(h_graph.lights);

        std::vector<Hittable*> d_light_ptrs(lights.size());
        for (size_t i = 0; i < lights.size(); i++) {
            d_light_ptrs[i] = hittables_registry.host_to_device(lights[i]);
        }

        Hittable** d_lights;
        size_t n_bytes = lights.size() * sizeof(Hittable*);
        CUDAMemory::allocate<Hittable*>(d_lights, lights.size());
        CUDAMemory::copy<Hittable*>(
            d_lights, d_light_ptrs.data(), lights.size()
        );

        h_graph.lights = d_lights;
    }

};

