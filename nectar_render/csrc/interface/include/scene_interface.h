#pragma once

#include <atomic>

#include "core/include/core.h"
#include "hittable/include/hittable/hittable.h"
#include "material/include/material/material.h"

#include "data/include/data.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/light.h"

uint8_t* selection_mask(
    size_t        H, 
    size_t        W,
    DeviceCamera* cam,
    SceneGraph*   scene,
    Hittable*     selected_object,
    int           outline_radius
);

struct SelectionMaskConfig {
    Color color = Color(0.98f, 0.75f, 0.17f);
    size_t outline_radius = (size_t)3;
};

class SceneInterface {
public:

    /* CONSTRUCTORS */

    ~SceneInterface() = default;
    SceneInterface() : rec(HitRecord()) { }

    /* CONFIGURATION */

    void configure(
        Scene*          current_scene,
        TransferStream* transfer_stream,
        HitRecord       hit_record
    ) { 
        scene  = current_scene; 
        stream = transfer_stream;
        rec    = hit_record; 
        enabled.store(true, std::memory_order_relaxed);
    }

    /* UPDATE HANDLING */

    void update(DeviceCamera* cam) {
        if (!is_enabled()) return;
        if (is_pending_teardown()) { teardown(); return; }
        build_selection_mask(cam);
    }

    /* STATE CONTROL */

    void disable() { 
        teardown_pending.store(true, std::memory_order_relaxed);
    }

    bool is_enabled() const { return enabled.load(std::memory_order_relaxed); }
    bool is_disabled() const { return !is_enabled(); }
    bool is_pending_teardown() const {
        return teardown_pending.load(std::memory_order_relaxed); 
    }
    
    /* PROPERTY ACCESS */

    Scene* get_scene() { return scene; }
    HitRecord& get_hit_record() { return rec; }

    /* TRANSFORM UTILS */

    Transform get_transform() { 
        return hit_object()->xform; 
    }

    void set_transform(const Transform& xform) {
        hit_object()->xform = xform;
    }

    /* MATERIAL UTILS */

    void update_material(const Material& material) {
        // object->material = material.build();
        return;
    }

    Material& get_material() {
        return scene->material_registry.get_material(rec.material_index);
    }

private:

    HitRecord rec;
    Scene* scene = nullptr;
    TransferStream* stream = nullptr;

    SelectionMaskConfig mask_cfg;

    std::atomic<bool> enabled { false };
    std::atomic<bool> teardown_pending { false };

    size_t material_index() { return rec.material_index; }
    uint32_t object_index() { return rec.object_index; }

    Hittable* hit_object() {
        return scene->hittables_registry.get_object(object_index());
    }

    Material& host_material() {
        return scene->material_registry.get_material(material_index());
    }

    void build_selection_mask(DeviceCamera* cam) {
        uint8_t* d_mask_ptr = selection_mask(
            stream->H, stream->W, cam, scene->graph, 
            rec.hit_object, mask_cfg.outline_radius
        );
        stream->overlay(d_mask_ptr, mask_cfg.color);
    }

    void teardown() {
        stream->remove_overlay();
        scene = nullptr; stream = nullptr; rec = HitRecord();
        teardown_pending.store(false, std::memory_order_relaxed);
        enabled.store(false, std::memory_order_relaxed);
    }
    
};

