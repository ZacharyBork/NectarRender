#pragma once

#include <algorithm>

#include "material/include/material/material.h"
#include "hittable/include/hittable/hittable.h"

inline const Material default_material = Material::lambertian(Color::purple());

class MaterialRegisty {
public:

    static constexpr size_t MAX_MATERIAL_COUNT = (size_t)256;

    __host__ ~MaterialRegisty() { }

    __host__ MaterialRegisty() { 
        h_materials.push_back(default_material);
        d_materials.push_back(default_material.build());
    }
    
    __host__ void register_materials(std::vector<Hittable*> hittables) {
        for (Hittable* obj : hittables) 
            register_material(obj);
        build_device_materials();
    }

    __host__ Material** device_materials() { return d_material_ptrs; }
    
    __host__ size_t material_count() { return h_materials.size(); }

    __host__ Material& get_material(size_t index) {
        return h_materials[index];
    }

    __host__ Material* get_device_ptr(size_t index) {
        return d_materials[index];
    }

    __host__ void destroy_device_materials() {
        for (Material& m : h_materials) { m.teardown(); }
        if (d_material_ptrs) cudaFree(d_material_ptrs);
        d_materials.clear();
    }

private:

    std::vector<Material>  h_materials{};
    std::vector<Material*> d_materials{};

    Material** d_material_ptrs;

    __host__ void register_material(Hittable* obj) {
        if (h_materials.size() >= MAX_MATERIAL_COUNT)
            throw std::runtime_error(
                "Maximum material count reached. Unable to register "
                "additional materials."
            );
        
        if (obj->material.material_type() == MaterialType::Null) {
            obj->material_index = (size_t)0; 
            return;
        }
        
        auto it = std::find(
            h_materials.begin(), h_materials.end(), obj->material
        );

        if (it != h_materials.end()) {
            obj->material_index = std::distance(h_materials.begin(), it);
            obj->material = Material();
        } else {
            obj->material_index = h_materials.size();
            h_materials.push_back(std::move(obj->material));
            d_materials.push_back(h_materials.back().build());
        }
    }

    __host__ void build_device_materials() {
        size_t n_bytes = material_count() * sizeof(Material*);
        cudaMalloc(&d_material_ptrs, n_bytes);
        cudaMemcpy(
            d_material_ptrs, d_materials.data(), 
            n_bytes, cudaMemcpyHostToDevice
        );
    }

    

};

