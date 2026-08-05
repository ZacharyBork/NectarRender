#pragma once

#include <atomic>

#include "core/include/core.h"
#include "hittable/include/hittable/hittable.h"
#include "material/include/material/material.h"
#include "light/include/skylight.h"

#include "data/include/data.h"
#include "engine/include/engine/trace.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/requests.h"

#include "scene.h"
#include "outliner.h"

uint8_t* selection_mask(
    size_t        H, 
    size_t        W,
    DeviceCamera* cam,
    SceneGraph*   scene,
    size_t        selected_object_id,
    int           outline_radius
);

struct SelectionMaskConfig {
    Color color = Color(0.98f, 0.75f, 0.17f);
    size_t outline_radius = (size_t)3;
};

enum class ToolState { SELECT, TRANSFORM };

class SceneInterface {
public:

    // CONSTRUCTORS ===========================================================

    ~SceneInterface() = default;    
    SceneInterface(
        Camera*         camera, 
        TransferStream* stream, 
        EngineRequests* requests
    ) : camera(camera), stream(stream), requests(requests) { }

    void update_scene(Scene* new_scene) { 
        scene = new_scene; outliner.set_scene(scene);
    }

    // UPDATE HANDLING ========================================================

    void update() {
        if (!is_enabled()) return;
        if (is_pending_teardown()) { teardown(); return; }
        build_selection_mask();
    }

    // OBJECT SELECTION =======================================================

    void query_scene(float u, float v) {
        std::lock_guard<std::mutex> lock(interface_mutex);

        HitRecord* d_rec;
        CUDAMemory::allocate<HitRecord>(d_rec);
        hit_test_ray(u, v, scene->graph, camera->device_camera(), d_rec);

        cudaMemcpy(&rec, d_rec, sizeof(HitRecord), cudaMemcpyDeviceToHost);
        CUDAMemory::free<HitRecord>(d_rec);

        if (rec.object_id == -1) return;
        if (is_disabled()) enable();
    }

    void select_scene_node(SceneNode node) {
        std::lock_guard<std::mutex> lock(interface_mutex);
        rec.object_id   = node.object_id;
        rec.material_id = node.material_id;
        if (rec.object_id == -1) return;
        if (is_disabled()) enable();
    }

    // STATE CONTROL ==========================================================

    void enable() {
        if (is_enabled()) return;
        enabled.store(true, relaxed);
    }

    void disable() { 
        teardown_pending.store(true, relaxed);
    }

    bool is_enabled() const { return enabled.load(relaxed); }
    bool is_disabled() const { return !is_enabled(); }
    bool is_pending_teardown() const {
        return teardown_pending.load(relaxed); 
    }

    // PROPERTY ACCESS ========================================================

    Scene* get_scene() { return scene; }
    HitRecord& get_hit_record() { return rec; }

    // SCENE OUTLINE ==========================================================

    SceneOutline get_scene_outline() {
        std::lock_guard<std::mutex> lock(interface_mutex);
        outliner.build_outline();
        return outliner.get_outline();
    }

    // SPAWNING / DELETING ====================================================

    void add_object(std::unique_ptr<Hittable> obj) { 
        std::lock_guard<std::mutex> lock(interface_mutex);
        scene->add_hittable(std::move(obj));
        scene->request_reset(true, true, false);
        requests->restart();
    }

    // TRANSFORM UTILS ========================================================

    Transform get_transform() { 
        std::lock_guard<std::mutex> lock(interface_mutex);
        return hit_object()->host_object->get_transform(); 
    }

    void set_transform(const Transform& xform) {
        std::lock_guard<std::mutex> lock(interface_mutex);
        hit_object()->host_object->set_transform(xform);
        hit_object()->mark_dirty();
        scene->request_reset(true, false, false);
        requests->restart();
    }

    // SKYLIGHT ===============================================================

    Skylight& get_skylight() { 
        std::lock_guard<std::mutex> lock(interface_mutex);
        return scene->skylight; }

    void request_skylight_update() {
        scene->request_reset(false, false, true);
        requests->restart();
    }

    void swap_skylight(Skylight new_skylight) {
        std::lock_guard<std::mutex> lock(interface_mutex);
        scene->skylight = std::move(new_skylight);
        scene->request_reset(false, false, true);
        requests->restart();
    }

    // MATERIAL UTILS =========================================================

    void set_material(Material mat) {
        // scene->material_registry.update_material(
        //     rec.material_id, std::move(mat)
        // );
        requests->restart();
    }

    Material& get_material() {
        return scene->material_registry.get_material(rec.material_id);
    }

private:

    Camera*         camera   = nullptr;
    TransferStream* stream   = nullptr;
    EngineRequests* requests = nullptr;
    Scene*          scene    = nullptr;

    HitRecord rec;
    SceneOutliner outliner;
    SelectionMaskConfig mask_cfg;
    ToolState tool_state = ToolState::SELECT;

    std::mutex interface_mutex;

    std::atomic<bool> enabled { false };
    std::atomic<bool> teardown_pending { false };

    size_t material_id() { return rec.material_id; }
    size_t object_id()   { return rec.object_id;   }

    HittableRegistryEntry* hit_object() {
        return scene->hittables_registry.get_entry(object_id());
    }

    Material& host_material() {
        return scene->material_registry.get_material(material_id());
    }

    void build_selection_mask() {
        uint8_t* d_mask_ptr = selection_mask(
            stream->H, stream->W, camera->device_camera(), scene->graph, 
            object_id(), mask_cfg.outline_radius
        );
        stream->overlay(d_mask_ptr, mask_cfg.color);
    }

    void teardown() {
        stream->remove_overlay();
        rec = HitRecord();
        teardown_pending.store(false, relaxed);
        enabled.store(false, relaxed);
    }
    
};

