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

private:

    std::vector<Material>  h_materials{};
    std::vector<Material*> d_materials{};

    Material** d_material_ptrs;

    __host__ void register_material(Hittable* obj) {
        if (h_materials.size() > MAX_MATERIAL_COUNT - 1u)
            throw std::runtime_error(
                "Maximum material count reached. Unable to register "
                "additional materials."
            );
        
        Material obj_mat = obj->material;
        if (obj_mat.material_type() == MaterialType::Null) {
            obj->material_index = (size_t)0; 
            return;
        }
        
        auto it = std::find(h_materials.begin(), h_materials.end(), obj_mat);

        if (it != h_materials.end()) {
            size_t index = std::distance(h_materials.begin(), it);
            obj->material_index = index;
        } else {
            size_t index = h_materials.size();
            h_materials.push_back(obj_mat);
            d_materials.push_back(obj_mat.build());
            obj->material_index = index;
        }
    }

    __host__ void build_device_materials() {
        Material* d_ptrs[MAX_MATERIAL_COUNT];

        for (uint32_t i = 0; i < d_materials.size(); i++) {
            d_ptrs[i] = d_materials[i];
        }

        size_t n_bytes = material_count() * sizeof(Material*);
        cudaMalloc(&d_material_ptrs, n_bytes);
        cudaMemcpy(
            d_material_ptrs, d_materials.data(), 
            n_bytes, cudaMemcpyHostToDevice
        );
    }

};

