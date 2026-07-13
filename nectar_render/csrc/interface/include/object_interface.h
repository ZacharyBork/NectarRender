#pragma once

#include "core/include/core.h"
#include "hittable/include/hittable/hittable.h"
#include "material/include/material/material.h"

class ObjectInterface {
public:

    __host__ ObjectInterface() 
        : rec(HitRecord()), 
          object("ObjectInterface::object")
    { }

    __host__ explicit ObjectInterface(HitRecord rec) 
        : rec(rec), object(rec.hit_object, "ObjectInterface::object")
    { }

    template<typename M>
    __host__ void update_material(const M& material) {
        object->update_material(material.build()); 
    }

    __host__ Material* get_material() {
        Material* h_mat_ptr;
        cudaMemcpy(
            &h_mat_ptr, object->material, sizeof(Material*), 
            cudaMemcpyDeviceToHost
        );
        return h_mat_ptr;
    }

    __host__ bool is_enabled() const { return object.is_enabled(); }
    __host__ HitRecord& hit_record() { return rec; }

private:

    HitRecord rec;
    Guarded<Hittable> object;

};

