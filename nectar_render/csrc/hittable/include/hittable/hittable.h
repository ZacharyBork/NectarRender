#pragma once

#include "hittable/include/bvh/aabb.h"
#include "material/include/material/material.h"

#include "hit_record.h"
#include "primitives.h"
#include "shapes.h"
#include "mesh.h"
#include "volumes.h"
#include "lights.h"

enum class HittableType : uint8_t { 
    Null, Quad, Sphere, Cube, Mesh, ConstantMedium, ObjectLight
};

inline std::string hittabletype_to_string(HittableType type) {
    switch (type) {
        case HittableType::Null:           return "Null";
        case HittableType::Quad:           return "Quad";
        case HittableType::Sphere:         return "Sphere";
        case HittableType::Cube:           return "Cube";
        case HittableType::Mesh:           return "Mesh";
        case HittableType::ConstantMedium: return "ConstantMedium";
        case HittableType::ObjectLight:    return "ObjectLight";
    }
}

#define FOR_EACH_HITTABLE_TYPE_HOST(X)   \
    X(Quad,   h_quad)                    \
    X(Sphere, h_sphere)                  \
    X(Cube,   h_cube)                    \
    X(Mesh,   h_mesh)                    \

#define FOR_EACH_HITTABLE_TYPE_DEVICE(X) \
    X(Quad,   d_quad)                    \
    X(Sphere, d_sphere)                  \
    X(Cube,   d_cube)                    \
    X(Mesh,   d_mesh)                    \

class Hittable {
private:

    HittableType type;
    union {
        Quad           h_quad;
        Sphere         h_sphere;
        Cube           h_cube;
        Mesh           h_mesh;
        ConstantMedium h_constant_medium;
        ObjectLight    h_object_light;
    };
    union {
        Quad*           d_quad;
        Sphere*         d_sphere;
        Cube*           d_cube;
        Mesh*           d_mesh;
        ConstantMedium* d_constant_medium;
        ObjectLight*    d_object_light;
    };

    __host__ void* core_ptr() const {
        switch (type) {
            case HittableType::Quad:           return d_quad;
            case HittableType::Sphere:         return d_sphere;
            case HittableType::Cube:           return d_cube;
            case HittableType::Mesh:           return d_mesh;
            case HittableType::ConstantMedium: return d_constant_medium;
            case HittableType::ObjectLight:    return d_object_light;
            default: return nullptr;
        }
    }

    AABB bbox;
    Material mat;
    Transform xform, delta;

    bool is_light_ = false;
    bool is_volumetric_ = false;

    size_t material_id = 0UL;
    size_t object_id   = 0UL;
    
    Hittable* wrapped_object = nullptr;

public:

    // CONSTRUCTORS ===========================================================

    __host__ ~Hittable() = default;
    __host__ Hittable(const Hittable&) = delete;

    __host__ Hittable(Hittable&& other) noexcept {
        std::memcpy(this, &other, sizeof(Hittable));
        other.type = HittableType::Null;
        other.mat = Material();
    }
    
    __host__ Hittable() : type(HittableType::Null) { }
    __host__ Hittable(HittableType type) : type(type) { }
    
    __device__ Hittable(
        HittableType type, 
        Transform    xform,
        Transform    delta,
        size_t       object_id,
        size_t       material_id,
        Hittable*    wrapped_object,
        void*        obj
    ) : type(type),
        xform(xform),
        delta(delta),
        object_id(object_id),
        material_id(material_id),
        wrapped_object(wrapped_object)
    { 
        switch (type) {
            case HittableType::Null: return;
            case HittableType::ConstantMedium: 
                d_constant_medium = reinterpret_cast<ConstantMedium*>(obj);
                break;
            case HittableType::ObjectLight: 
                d_object_light = reinterpret_cast<ObjectLight*>(obj);
                break;
            #define X(Name, Member) case HittableType::Name: \
                Member = reinterpret_cast<Name*>(obj); break;
            FOR_EACH_HITTABLE_TYPE_DEVICE(X)
            #undef X
        }
    }

    // OPERATORS ==============================================================

    __host__ Hittable& operator=(Hittable&& other) noexcept {
        if (this != &other) {
            std::memcpy(this, &other, sizeof(Hittable));
            other.type = HittableType::Null;
            other.mat = Material();
        }
        return *this;
    }

    __host__ Hittable& operator=(const Hittable&) = delete;

    // QUAD ===================================================================

    __host__ static Hittable quad(
        const Vector3 position,
        const Vector3 rotation,
        const Vector3 scale,
        Material material
    ) {
        Hittable obj(HittableType::Quad); 
        obj.h_quad = Quad(); 
        obj.d_quad = obj.h_quad.build();

        obj.xform = Transform(position, rotation, scale);
        obj.mat   = std::move(material);
        obj.bbox  = obj.h_quad.build_bbox();
        return obj;
    }

    // CUBE ===================================================================

    __host__ static Hittable cube(
        const Vector3 position,
        const Vector3 rotation,
        const Vector3 scale,
        Material material
    ) {
        Hittable obj(HittableType::Cube); 
        obj.h_cube = Cube();
        obj.d_cube = obj.h_cube.build();

        obj.xform = Transform(position, rotation, scale);
        obj.mat   = std::move(material);
        obj.bbox  = obj.h_cube.build_bbox();
        return obj;
    }

    // SPHERE =================================================================

    __host__ static Hittable sphere(
        const Vector3 position,
        const float radius,
        Material material
    ) {
        Hittable obj(HittableType::Sphere); 
        obj.h_sphere = Sphere(radius); 
        obj.d_sphere = obj.h_sphere.build();
        
        obj.xform = Transform(position);
        obj.mat   = std::move(material);
        obj.bbox  = obj.h_sphere.build_bbox();
        return obj;
    }

    // MESH ===================================================================

    __host__ static Hittable mesh(
        const std::string& path,
        const Vector3 position,
        const Vector3 rotation,
        const Vector3 scale,
        Material material
    ) {
        Hittable obj(HittableType::Mesh); 
        obj.h_mesh = Mesh();
        obj.h_mesh.load_obj(path);
        obj.d_mesh = obj.h_mesh.build();

        obj.xform = Transform(position, rotation, scale);
        obj.mat   = std::move(material);
        obj.bbox  = obj.h_mesh.build_bbox();
        return obj;
    }

    // CONSTANT MEDIUM ========================================================

    __host__ static Hittable constant_medium(
        Hittable& bound_obj,
        float density,
        std::shared_ptr<Texture> texture
    ) {
        Hittable obj(HittableType::ConstantMedium); 
        obj.h_constant_medium = ConstantMedium(density);
        obj.d_constant_medium = device_build<ConstantMedium>(density);
        obj.wrapped_object    = bound_obj.build();

        obj.xform = bound_obj.xform;
        obj.delta = bound_obj.delta;
        obj.bbox  = bound_obj.bbox;
        obj.mat   = std::move(Material::isotropic(texture));
        obj.is_volumetric_ = true;
        return obj;
    }

    __host__ static Hittable constant_medium(
        Hittable& bound_obj,
        float density,
        const Color& albedo
    ) {
        return Hittable::constant_medium(
            bound_obj, density, Texture::from_color(albedo)
        );
    }

    // OBJECT LIGHT ===========================================================

    __host__ static Hittable object_light(
        Hittable& bound_obj,
        float brightness,
        std::shared_ptr<Texture> texture
    ) {
        Hittable obj(HittableType::ObjectLight); 
        obj.h_object_light = ObjectLight();
        obj.d_object_light = device_build<ObjectLight>();
        obj.wrapped_object = bound_obj.build();

        obj.xform = bound_obj.xform;
        obj.delta = bound_obj.delta;
        obj.bbox  = bound_obj.bbox;
        obj.mat   = std::move(Material::emissive(texture, brightness));
        obj.is_light_ = true;
        return obj;
    }

    __host__ static Hittable object_light(
        Hittable& bound_obj,
        float brightness,
        const Color& albedo
    ) {
        return Hittable::object_light(
            bound_obj, brightness, Texture::from_color(albedo)
        );
    }

    // UTILITIES ==============================================================

    __host__ Hittable* build() {
        return device_build<Hittable>(
            type, xform, delta, object_id, material_id, 
            wrapped_object, core_ptr()
        );
    }

    __host__ void teardown() { if (core_ptr()) CUDAMemory::free(core_ptr()); }

    __host__ HittableType hittable_type() const { return type;           }
    __host__ bool is_light()              const { return is_light_;      }
    __host__ bool is_volumetric()         const { return is_volumetric_; }

    __host__ const AABB bounding_box() const { 
        return bbox.transformed(xform);
    }

    // MATERIAL UTILS =========================================================

    __host__ Material& get_material() { return mat; }
    __host__ void set_material(Material material) { 
        mat = std::move(material); 
    }

    // TRANSFORM UTILS ========================================================

    __host__ Transform get_transform() const { return xform; }
    __host__ void set_transform(Transform t) { xform = t;    }

    // ID UTILS ===============================================================

    __host__ void set_material_id(size_t idx) { material_id = idx; }
    __host__ __device__ size_t get_material_id() const { 
        return material_id; 
    }

    __host__ void set_object_id(size_t idx) { object_id = idx; }
    __host__ __device__ size_t get_object_id() const { 
        return object_id; 
    }

    // HIT TESTING ============================================================

    __device__ bool hit_test(const Ray& ray, HitRecord& rec) const {
        Ray r = ray.to_object_space(xform);
        bool hit_obj = hit(r, Interval(EPS, FMAX), rec);
        rec.to_world_space(xform, r, ray, true);

        return hit_obj;
    }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const {
        switch (type) {
            case HittableType::ObjectLight:
                return d_object_light->hit(
                    ray, ray_t, rec, wrapped_object
                );
            case HittableType::ConstantMedium:
                return d_constant_medium->hit(
                    ray, ray_t, rec, wrapped_object
                );
            #define X(Name, Member) case HittableType::Name: \
                return Member->hit(ray, ray_t, rec, xform);
            FOR_EACH_HITTABLE_TYPE_DEVICE(X)
            #undef X
        }
        return false;
    }

    // PDF ====================================================================

    __device__ float pdf_value(
        const Vector3& origin,
        const Vector3& direction
    ) const {
        switch (type) {
            case HittableType::ObjectLight:
                return d_object_light->pdf_value(
                    origin, direction, wrapped_object
                );
            #define X(Name, Member) case HittableType::Name: \
                return Member->pdf_value(origin, direction, xform);
            FOR_EACH_HITTABLE_TYPE_DEVICE(X)
            #undef X
        }
        return 0.0f;
    }

    __device__ Vector3 random(
        const Vector3& origin,
        Generator& gen
    ) const {
        switch (type) {
            case HittableType::ObjectLight:
                return d_object_light->random(origin, gen, wrapped_object);
            #define X(Name, Member) case HittableType::Name: \
                return Member->random(origin, gen, xform);
            FOR_EACH_HITTABLE_TYPE_DEVICE(X)
            #undef X
        }
        return Vector3(1.0f, 0.0f, 0.0f);
    }

    // MOTION =================================================================

    __host__ void set_motion_vector(const Vector3& offset) {
        delta.set_position(offset);
    }

    __device__ Vector3 position_at_time(float time) const {
        if (delta.p().length() < 1e-16) return xform.p();
        Ray motion(xform.p(), delta.p());
        return motion.at(time);
    }



};



