#pragma once

#include <unordered_map>
#include "hittable/include/hittable/hittable.h"
#include "hittable/include/bvh/node.h"

class HittablesRegistry {
public:

    BVHNode* bvh_nodes = nullptr;

    __host__ HittablesRegistry() { } 
    __host__ HittablesRegistry(std::vector<Hittable*> hittables) 
        : h_hittables(std::move(hittables)) { }

    __host__ void build() {
        build_bvh();
        build_device_hittables();
    }

    __host__ Hittable* host_to_device(Hittable* h_ptr) {
        auto it = map_host_to_device.find(h_ptr);
        if (it == map_host_to_device.end())
            throw std::runtime_error(
                "Object instance not found among traversable hittables."
            );
        return it->second;
    }

    __host__ void destroy_device_hittables() {
        for (Hittable* obj : d_hittables) { if (obj) cudaFree(obj); }
        if (d_hittables_ptrs) cudaFree(d_hittables_ptrs);
        d_hittables.clear();
        map_host_to_device.clear();
    }

    __host__ void rebuild() { 
        if (d_hittables_ptrs) destroy_device_hittables();
        h_hittables.clear();
        build(); 
    }

    __host__ size_t object_count()   { return n_objects; }
    __host__ size_t bvh_node_count() { return n_bvh_nodes; }

    __host__ Hittable** device_hittables()    { return d_hittables_ptrs; }
    __host__ std::vector<Hittable*> objects() { return h_hittables; }
    __host__ Hittable* get_object(size_t index) {
        if (index >= object_count())
            throw std::runtime_error(
                "HittablesRegistry::get_object(): index " + 
                std::to_string(index) + " out of range"
            );
        return h_hittables[index];
    }

private:

    size_t n_objects   = (size_t)0;
    size_t n_bvh_nodes = (size_t)0;

    std::vector<Hittable*> h_hittables{};
    std::vector<Hittable*> d_hittables{};
    
    Hittable** d_hittables_ptrs = nullptr;
    std::unordered_map<Hittable*, Hittable*> map_host_to_device;

    __host__ void build_bvh() {
        BVH bvh;
        bvh.build(h_hittables);
        n_bvh_nodes = bvh.nodes.size();

        size_t n_bytes = n_bvh_nodes * sizeof(BVHNode);
        cudaMalloc(&bvh_nodes, n_bytes);
        cudaMemcpy(
            bvh_nodes, bvh.nodes.data(),
            n_bytes, cudaMemcpyHostToDevice
        );
    }

    __host__ void register_hittable(Hittable* obj) {
        obj->object_index = h_hittables.size();
        h_hittables.push_back(obj);
        d_hittables.push_back(obj->build());
    }

    __host__ void build_device_hittables() {
        n_objects = h_hittables.size();

        d_hittables.clear();
        d_hittables.reserve(n_objects);
        map_host_to_device.clear();

        for (size_t i = 0; i < n_objects; i++) {
            h_hittables[i]->object_index = i;
            Hittable* d_ptr = h_hittables[i]->build();
            d_hittables.push_back(d_ptr);
            map_host_to_device[h_hittables[i]] = d_ptr;
        }

        size_t n_bytes = d_hittables.size() * sizeof(Hittable*);
        cudaMalloc(&d_hittables_ptrs, n_bytes);
        cudaMemcpy(
            d_hittables_ptrs, d_hittables.data(), 
            n_bytes, cudaMemcpyHostToDevice
        );
    }

};


