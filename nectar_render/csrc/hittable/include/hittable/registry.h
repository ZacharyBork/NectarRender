#pragma once

#include <atomic>
#include <unordered_map>

#include "hittable/include/hittable/hittable.h"
#include "hittable/include/bvh/node.h"

struct HittableRegistryEntry {
public:

    Hittable* host_object;
    Hittable* d_ptr = nullptr;
    
    bool is_light = false;
    HittableType type = HittableType::Null;
        
    size_t object_id = 0UL;
    size_t material_index = 0UL;
    std::string tag = "";
    
    // CONSTRUCTORS / DESTRUCTORS =============================================

    __host__ HittableRegistryEntry() = default;
    __host__ HittableRegistryEntry(Hittable* obj) : host_object(obj) { }

    __host__ ~HittableRegistryEntry() { if (d_ptr) CUDAMemory::free(d_ptr); }

    __host__ HittableRegistryEntry(const HittableRegistryEntry&) = delete;
    __host__ HittableRegistryEntry& operator=(
        const HittableRegistryEntry&
    ) = delete;

    __host__ HittableRegistryEntry(HittableRegistryEntry&& other) noexcept
      : host_object(other.host_object), 
        d_ptr(other.d_ptr),
        is_light(other.is_light), 
        type(other.type),
        object_id(other.object_id), 
        material_index(other.material_index),
        tag(std::move(other.tag))
    {
        rebuild_pending.store(other.rebuild_pending.load(relaxed), relaxed);
        other.d_ptr = nullptr;
        other.host_object = nullptr;
    }

    __host__ HittableRegistryEntry& operator=(
        HittableRegistryEntry&& other
    ) noexcept {
        if (this != &other) {
            if (d_ptr) CUDAMemory::free(d_ptr);
            host_object    = other.host_object;
            d_ptr          = other.d_ptr;
            is_light       = other.is_light;
            type           = other.type;
            object_id      = other.object_id;
            material_index = other.material_index;
            tag            = std::move(other.tag);
            rebuild_pending.store(
                other.rebuild_pending.load(relaxed), relaxed
            );
            
            other.d_ptr = nullptr;
            other.host_object = nullptr;
        }
        return *this;
    }

    // DEVICE BUILD / DESTROY =================================================
    
    __host__ void build() {
        if (d_ptr && !rebuild_pending.exchange(false, relaxed)) return;
        if (!d_ptr || rebuild_pending.load(relaxed))
        destroy_device_hittable();

        d_ptr    = host_object->build();
        is_light = host_object->is_light();
        type     = host_object->hittable_type();

        object_id      = host_object->get_object_id();
        material_index = host_object->get_material_index();
    }

    __host__ void destroy_device_hittable() {
        if (d_ptr) { CUDAMemory::free(d_ptr); d_ptr = nullptr; }
    }

    // UTILITIES ==============================================================

    __host__ void mark_dirty() { rebuild_pending.store(true, relaxed); }

private:

    std::atomic<bool> rebuild_pending { false };

};

class HittablesRegistry {
public:

    static constexpr size_t MAX_REGISTRY_ENTRIES = 1024UL;

    // CONSTRUCTORS ===========================================================

    __host__ HittablesRegistry() { } 
    __host__ HittablesRegistry(
        std::vector<Hittable*>& hittables,
        std::vector<Hittable*>& lights
    ) {
        n_lights = lights.size(); 
        for (Hittable* light : lights) hittables.push_back(light);
        n_objects = hittables.size();
        register_hittables(hittables);
    }

    // DEVICE BUILD / DESTROY =================================================

    __host__ void build() {
        build_bvh();
        build_device_hittables();
    }

    __host__ void destroy_device_hittables() {
        for (size_t i = 0UL; i < n_objects; i++) 
            entries[i].destroy_device_hittable();

        if (d_hittables_ptrs) CUDAMemory::free(d_hittables_ptrs);
        if (bvh_nodes) CUDAMemory::free(bvh_nodes);
        
        d_hittables_ptrs = nullptr;
        bvh_nodes = nullptr;
    }

    // INSPECTION =============================================================

    __host__ size_t object_count()   { return n_objects;   }
    __host__ size_t light_count()    { return n_lights;    }
    __host__ size_t bvh_node_count() { return n_bvh_nodes; }

    __host__ BVHNode*   device_bvh_nodes() { return bvh_nodes;        }
    __host__ Hittable** device_lights()    { return d_lights_ptrs;    }
    __host__ Hittable** device_hittables() { return d_hittables_ptrs; }
    
    // REGISTRY ACCESS ========================================================

    __host__ std::vector<Hittable*> objects() { 
        std::vector<Hittable*> objs;
        objs.reserve(n_objects);
        for (size_t i = 0UL; i < n_objects; i++) 
            objs.push_back(entries[i].host_object);
        return objs;
    }

    __host__ HittableRegistryEntry* get_entry(size_t index) {
        auto it = index_to_object.find(index);
        if (it == index_to_object.end())
            throw std::runtime_error("Object index not valid.");
        return it->second;
    }

    // UTILITIES ==============================================================

    void for_each_entry(std::function<void(HittableRegistryEntry&)> func) {
        for (size_t i = 0UL; i < n_objects; i++) func(entries[i]);
    }

private:

    BVH<HittableRegistryEntry> bvh;

    size_t n_objects   = 0UL;
    size_t n_lights    = 0UL;
    size_t n_bvh_nodes = 0UL;

    HittableRegistryEntry entries[MAX_REGISTRY_ENTRIES];
    
    BVHNode*   bvh_nodes = nullptr;
    Hittable** d_lights_ptrs = nullptr;
    Hittable** d_hittables_ptrs = nullptr;
    
    std::unordered_map<size_t, HittableRegistryEntry*> index_to_object;

    __host__ void register_hittables(std::vector<Hittable*> hittables) {
        for (size_t i = 0UL; i < hittables.size(); i++) {
            if (i > MAX_REGISTRY_ENTRIES) 
                throw std::runtime_error(
                    "Hittables registry exceeded maximum registry entries ["
                    + std::to_string(MAX_REGISTRY_ENTRIES) + "]. Exiting..."
                );
            
            entries[i] = HittableRegistryEntry{ hittables[i] };
            entries[i].host_object->set_object_id(i);
            entries[i].object_id = i;
        }
    }

    __host__ void build_bvh() {
        std::vector<HittableRegistryEntry> hittables;
        hittables.reserve(n_objects);
        for (size_t i = 0UL; i < n_objects; i++)
            hittables.push_back(std::move(entries[i]));

        bvh.build(std::move(hittables), [](HittableRegistryEntry& h){ 
            return h.host_object->bounding_box(); 
        });
        n_bvh_nodes = bvh.nodes.size();
        for (size_t i = 0UL; i < n_objects; i++) {
            entries[i] = std::move(bvh.items[i]);
            index_to_object[entries[i].object_id] = &entries[i];
        }

        if (!bvh_nodes) CUDAMemory::allocate<BVHNode>(bvh_nodes, n_bvh_nodes);
        CUDAMemory::copy<BVHNode>(bvh_nodes, bvh.nodes.data(), n_bvh_nodes);
    }

    __host__ void build_device_hittables() {
        std::vector<Hittable*> d_lights;    d_lights.reserve(n_lights);
        std::vector<Hittable*> d_hittables; d_hittables.reserve(n_objects);

        for (size_t i = 0UL; i < n_objects; i++) {
            HittableRegistryEntry& entry = entries[i];
            entry.build();
            d_hittables.push_back(entry.d_ptr);
            if (entry.is_light) d_lights.push_back(entry.d_ptr);
        }

        if (!d_hittables_ptrs)
            CUDAMemory::allocate<Hittable*>(d_hittables_ptrs, n_objects);
        CUDAMemory::copy<Hittable*>(
            d_hittables_ptrs, d_hittables.data(), n_objects
        );

        if (!d_lights_ptrs)
            CUDAMemory::allocate<Hittable*>(d_lights_ptrs, n_lights);
        CUDAMemory::copy<Hittable*>(d_lights_ptrs, d_lights.data(), n_lights);
    }

};


