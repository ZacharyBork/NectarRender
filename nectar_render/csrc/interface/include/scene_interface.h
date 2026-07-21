#pragma once

#include "core/include/core.h"
#include "hittable/include/hittable/hittable.h"
#include "material/include/material/material.h"

#include "engine/include/engine/data.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/light.h"

void selection_mask(
    DataView      data,
    DeviceCamera* cam,
    SceneGraph*   scene,
    Hittable*     selected_object,
    int      outline_radius,
    uint8_t  r, 
    uint8_t  g,
    uint8_t  b
);

class SceneInterface {
public:

    /* CONSTRUCTORS */

    __host__ SceneInterface() : rec(HitRecord()) { }

    __host__ explicit SceneInterface(Scene* scene, HitRecord rec) 
      : scene(scene), rec(rec) { }

    /* PROPERTY ACCESS */

    __host__ bool is_enabled() const { return scene != nullptr; }
    __host__ HitRecord& hit_record() { return rec; }

    /* TRANSFORM UTILS */

    __host__ Transform get_transform() { 
        return hit_object()->xform; 
    }

    __host__ void set_transform(const Transform& xform) {
        hit_object()->xform = xform;
    }

    /* MATERIAL UTILS */

    __host__ void update_material(const Material& material) {
        // object->material = material.build();
        return;
    }

    __host__ Material& get_material() {
        return scene->material_registry.get_material(rec.material_index);
    }

    /* SELECTION MASKING */

    __host__ void build_selection_mask(
        size_t H, size_t W,
        DeviceCamera* cam,
        SceneGraph*   scene,
        DataObject&   data,
        const Color&  color = Color::white(),
        int outline_radius = 3
    ) {
        if (!is_enabled()) return;
        selection_mask(
            data.view(), cam, scene, rec.hit_object, 
            outline_radius, 255u, 120u, 45u
        );
    }

    /* STATE MANAGEMENT */

    __host__ void disable() { rec = HitRecord(); scene = nullptr; }

private:

    HitRecord rec;
    Scene* scene = nullptr;

    __host__ size_t material_index() { return rec.material_index; }
    __host__ uint32_t object_index() { return rec.object_index; }

    __host__ Hittable* hit_object() {
        return scene->hittables_registry.get_object(object_index());
    }

    __host__ Material& host_material() {
        return scene->material_registry.get_material(material_index());
    }
    
};

