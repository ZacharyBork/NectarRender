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

class ObjectInterface {
public:

    /* CONSTRUCTORS */

    __host__ ObjectInterface() 
        : rec(HitRecord()), 
          object("ObjectInterface::object")
    { }

    __host__ explicit ObjectInterface(Scene* scene, HitRecord rec) 
        : scene(scene), rec(rec), 
          object(
            scene->object_at_index(rec.object_index), 
            "ObjectInterface::object"
        )
    { }

    /* PROPERTY ACCESS */

    __host__ bool is_enabled() const { return object.is_enabled(); }
    __host__ HitRecord& hit_record() { return rec; }

    /* TRANSFORM UTILS */

    __host__ Transform get_transform() { 
        return object->xform; 
    }

    __host__ void set_transform(const Transform& xform) {
        object->xform = xform;
    }

    /* MATERIAL UTILS */

    template<typename M>
    __host__ void update_material(const M& material) {
        object->material = material.build();
    }

    __host__ Material* get_material() {
        Material* h_mat_ptr;
        cudaMemcpy(
            &h_mat_ptr, object->material, sizeof(Material*), 
            cudaMemcpyDeviceToHost
        );
        return h_mat_ptr;
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
        selection_mask(
            data.view(), cam, scene, rec.hit_object, 
            outline_radius, 255u, 120u, 45u
        );
    }

    /* STATE MANAGEMENT */

    __host__ void disable() {
        rec = HitRecord();
        object.disable();
    }

private:

    Scene* scene = nullptr;
    HitRecord rec;
    Guarded<Hittable> object;

};

