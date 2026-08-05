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
        
    size_t object_id   = 0UL;
    size_t parent_id   = 0UL;
    size_t material_id = 0UL;

    std::string name = "";
    
    // CONSTRUCTORS / DESTRUCTORS =============================================

    HittableRegistryEntry() = default;
    HittableRegistryEntry(Hittable* obj) : host_object(obj) { }

    HittableRegistryEntry(const HittableRegistryEntry&) = delete;
    HittableRegistryEntry& operator=(
        const HittableRegistryEntry&
    ) = delete;

    HittableRegistryEntry(HittableRegistryEntry&& other) noexcept
      : host_object(other.host_object), 
        d_ptr(other.d_ptr),
        is_light(other.is_light), 
        type(other.type),
        object_id(other.object_id), 
        material_id(other.material_id),
        name(std::move(other.name))
    {
        rebuild_pending.store(other.rebuild_pending.load(relaxed), relaxed);
        other.d_ptr = nullptr;
        other.host_object = nullptr;
    }

    HittableRegistryEntry& operator=(
        HittableRegistryEntry&& other
    ) noexcept {
        if (this != &other) {
            if (d_ptr) CUDAMemory::free(d_ptr);
            host_object = other.host_object;
            d_ptr       = other.d_ptr;
            is_light    = other.is_light;
            type        = other.type;
            object_id   = other.object_id;
            material_id = other.material_id;
            name        = std::move(other.name);
            rebuild_pending.store(
                other.rebuild_pending.load(relaxed), relaxed
            );
            
            other.d_ptr = nullptr;
            other.host_object = nullptr;
        }
        return *this;
    }

    // DEVICE BUILD / DESTROY =================================================
    
    void build() {
        if (d_ptr && !rebuild_pending.load(relaxed)) return;
        destroy_device_hittable();
        rebuild_pending.exchange(false, relaxed);

        d_ptr    = host_object->build();
        is_light = host_object->is_light();
        type     = host_object->hittable_type();

        object_id   = host_object->get_object_id();
        material_id = host_object->get_material_id();

        name = hittabletype_to_string(type);
    }

    void destroy_device_hittable() {
        if (d_ptr) { CUDAMemory::free(d_ptr); d_ptr = nullptr; }
    }

    // UTILITIES ==============================================================

    void mark_dirty() { rebuild_pending.store(true, relaxed); }

private:

    std::atomic<bool> rebuild_pending { false };

};

class HittablesRegistry {
public:

    static constexpr size_t MAX_REGISTRY_ENTRIES = 1024UL;

    // CONSTRUCTORS ===========================================================

    HittablesRegistry() { } 
    
    // REGISTRATION ===========================================================

    void register_hittable(Hittable* hittable) {
        if (n_objects > MAX_REGISTRY_ENTRIES) 
            throw std::runtime_error(
                "Hittables registry exceeded maximum registry entries ["
                + std::to_string(MAX_REGISTRY_ENTRIES) + "]. Exiting..."
            );
        
        entries[n_objects] = HittableRegistryEntry{ hittable };
        entries[n_objects].host_object->set_object_id(n_objects);
        entries[n_objects].object_id = n_objects;
        n_objects++;
    }

    void register_hittable(std::unique_ptr<Hittable> hittable) {
        if (n_objects > MAX_REGISTRY_ENTRIES) 
            throw std::runtime_error(
                "Hittables registry exceeded maximum registry entries ["
                + std::to_string(MAX_REGISTRY_ENTRIES) + "]. Exiting..."
            );

        Hittable* raw = hittable.get();
        owned_hittables.push_back(std::move(hittable));

        entries[n_objects] = HittableRegistryEntry{ raw };
        entries[n_objects].host_object->set_object_id(n_objects);
        entries[n_objects].object_id = n_objects;
        n_objects++;
    }

    void register_hittables(std::vector<Hittable*>& hittables) {
        for (size_t i = 0UL; i < hittables.size(); i++) {
            register_hittable(hittables[i]);
        }
    }

    // DEVICE BUILD / DESTROY =================================================

    void build() {
        build_bvh();
        build_device_hittables();
    }

    void destroy_device_hittables() {
        for (size_t i = 0UL; i < n_objects; i++) 
            entries[i].destroy_device_hittable();

        if (d_hittables_ptrs) CUDAMemory::free(d_hittables_ptrs);
        if (bvh_nodes) CUDAMemory::free(bvh_nodes);
        
        d_hittables_ptrs = nullptr;
        bvh_nodes = nullptr;
    }

    // INSPECTION =============================================================

    size_t object_count()   { return n_objects;   }
    size_t bvh_node_count() { return n_bvh_nodes; }

    BVHNode*   device_bvh_nodes() { return bvh_nodes;        }
    Hittable** device_lights()    { return d_lights_ptrs;    }
    Hittable** device_hittables() { return d_hittables_ptrs; }
    
    // REGISTRY ACCESS ========================================================

    std::vector<Hittable*> objects() { 
        std::vector<Hittable*> objs;
        objs.reserve(n_objects);
        for (size_t i = 0UL; i < n_objects; i++) 
            objs.push_back(entries[i].host_object);
        return objs;
    }

    HittableRegistryEntry* get_entry(size_t index) {
        auto it = index_to_object.find(index);
        if (it == index_to_object.end())
            throw std::runtime_error("Object index not valid.");
        return it->second;
    }

    std::vector<HittableRegistryEntry*> get_all_entries() {
        std::vector<HittableRegistryEntry*> all_entries;
        all_entries.reserve(n_objects);

        for (size_t i = 0UL; i < n_objects; i++)
            all_entries.push_back(&entries[i]);
        
        return all_entries;
    }

private:

    HittableRegistryEntry entries[MAX_REGISTRY_ENTRIES]; 
    std::vector<std::unique_ptr<Hittable>> owned_hittables;   
    BVH<HittableRegistryEntry> bvh;
    
    size_t n_objects   = 0UL;
    size_t n_bvh_nodes = 0UL;
    
    BVHNode*   bvh_nodes        = nullptr;
    Hittable** d_lights_ptrs    = nullptr;
    Hittable** d_hittables_ptrs = nullptr;
    
    std::unordered_map<size_t, HittableRegistryEntry*> index_to_object;

    void build_bvh() {
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

        if (bvh_nodes) { CUDAMemory::free(bvh_nodes); bvh_nodes = nullptr; }
        CUDAMemory::allocate<BVHNode>(bvh_nodes, n_bvh_nodes);
        CUDAMemory::copy<BVHNode>(bvh_nodes, bvh.nodes.data(), n_bvh_nodes);
    }

    void build_device_hittables() {
        std::vector<Hittable*> d_lights;
        std::vector<Hittable*> d_hittables; d_hittables.reserve(n_objects);

        for (size_t i = 0UL; i < n_objects; i++) {
            entries[i].build(); 
            d_hittables.push_back(entries[i].d_ptr);
            if (entries[i].is_light) d_lights.push_back(entries[i].d_ptr);
        }

        if (d_hittables_ptrs) { 
            CUDAMemory::free(d_hittables_ptrs);
            d_hittables_ptrs = nullptr; 
        }
        CUDAMemory::allocate<Hittable*>(d_hittables_ptrs, n_objects);
        CUDAMemory::copy<Hittable*>(
            d_hittables_ptrs, d_hittables.data(), n_objects
        );

        size_t n_lights = d_lights.size();
        if (n_lights == 0UL) return;
        if (d_lights_ptrs) { 
            CUDAMemory::free(d_lights_ptrs);
            d_lights_ptrs = nullptr; 
        }
        CUDAMemory::allocate<Hittable*>(d_lights_ptrs, n_lights);
        CUDAMemory::copy<Hittable*>(d_lights_ptrs, d_lights.data(), n_lights);
    }

};


