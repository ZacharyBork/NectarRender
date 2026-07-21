#pragma once

#include <unordered_map>
#include "hittable/include/hittable/hittable.h"
#include "hittable/include/bvh/node.h"

class HittablesRegistry {
public:

    __host__ HittablesRegistry() { }

    __host__ void register_hittables(std::vector<Hittable*> hittables) {
        for (Hittable* obj : hittables) 
            register_hittable(obj);
        build_device_hittables();
    }

    __host__ size_t     hittables_count()  { return object_count; }
    __host__ Hittable** device_hittables() { return d_hittables_ptrs; }

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
        register_hittables(h_hittables); 
    }

private:

    size_t object_count   = (size_t)0;
    size_t bvh_node_count = (size_t)0;

    std::vector<Hittable*> h_hittables{};
    std::vector<Hittable*> d_hittables{};

    Hittable** d_hittables_ptrs = nullptr;
    std::unordered_map<Hittable*, Hittable*> map_host_to_device;

    __host__ void register_hittable(Hittable* obj) {
        obj->object_index = h_hittables.size();
        h_hittables.push_back(obj);
        d_hittables.push_back(obj->build());
    }

    __host__ void build_device_hittables() {
        object_count = d_hittables.size();
        for (Hittable* obj : h_hittables) {
            d_hittables.push_back(obj->build());
            map_host_to_device[obj] = d_hittables.back();
        }

        size_t n_bytes = object_count * sizeof(Hittable*);
        cudaMalloc(&d_hittables_ptrs, n_bytes);
        cudaMemcpy(
            d_hittables_ptrs, d_hittables.data(),
            n_bytes, cudaMemcpyHostToDevice
        );
    }

};


