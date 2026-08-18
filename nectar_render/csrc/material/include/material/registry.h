#pragma once

#include <algorithm>

#include "material/include/material/material.h"
#include "hittable/include/hittable/hittable.h"

class MaterialRegisty {
public:

    static constexpr size_t MAX_MATERIAL_COUNT = 4096UL;

    // CONSTRUCTORS / DESTRUCTORS =============================================

    __host__ ~MaterialRegisty() { }

    __host__ MaterialRegisty(const MaterialRegisty&) = delete;
    __host__ MaterialRegisty& operator=(const MaterialRegisty&) = delete;

    __host__ MaterialRegisty(MaterialRegisty&&) noexcept = default;
    __host__ MaterialRegisty& operator=(MaterialRegisty&&) noexcept = default;

    __host__ MaterialRegisty() {
        h_materials.push_back(make_default_material());
    }

    // REGISTRATION ===========================================================

    __host__ void register_material(Hittable* obj) {
        if (obj->get_material().material_type() == MaterialType::Null) {
            obj->set_material_id((size_t)0);
            return;
        }

        if (h_materials.size() >= MAX_MATERIAL_COUNT)
            throw std::runtime_error(
                "Maximum material count reached. Unable to register "
                "additional materials."
            );

        obj->set_material_id(h_materials.size());
        h_materials.push_back(std::move(obj->get_material()));
    }

    __host__ void register_materials(std::vector<Hittable*> hittables) {
        for (Hittable* obj : hittables) register_material(obj);
    }

    // PROPERTY ACCESS ========================================================

    __host__ Material** device_materials() { return d_material_ptrs;    }
    __host__ size_t material_count()       { return h_materials.size(); }
    
    __host__ Material& get_material(size_t index) { 
        return h_materials[index]; 
    }
    
    __host__ Material* get_device_ptr(size_t index) { 
        return d_materials[index];
    }

    // UPDATING  ==============================================================

    __host__ void update_material(size_t index, Material new_material) { 
        h_materials[index].teardown();
        h_materials[index] = std::move(new_material);        
    }

    // BUILD / DESTROY ========================================================

    __host__ void destroy_device_materials() {
        if (!d_material_ptrs) return;
        for (Material* m : d_materials) { if (m) CUDAMemory::free(m);  }
        
        CUDAMemory::free(d_material_ptrs);
        d_material_ptrs = nullptr;
        d_materials.clear();
    }

    __host__ void teardown() {
        destroy_device_materials();
        for (Material& m : h_materials) { m.teardown(); }
    }

    __host__ void build_device_materials() {
        for (Material& mat : h_materials)
            d_materials.push_back(mat.build());

        size_t n_bytes = material_count() * sizeof(Material*);
        cudaMalloc(&d_material_ptrs, n_bytes);
        cudaMemcpy(
            d_material_ptrs, d_materials.data(), 
            n_bytes, cudaMemcpyHostToDevice
        );
    }

private:

    std::vector<Material>  h_materials{};
    std::vector<Material*> d_materials{};
    Material** d_material_ptrs = nullptr;
  
};

