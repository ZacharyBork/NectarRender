#pragma once

#include "tiny_obj_loader.h"

#include "core/include/core.h"
#include "hittable/include/bvh/node.h"
#include "hittable/include/hittable/hittable.h"

// ============================================================================
// OBJ LOADING
// ============================================================================

struct TriangleRef { uint32_t v0, v1, v2; };
struct MeshVertex {
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    Vector2 uv;
};

struct MeshLoadResult {
    std::vector<MeshVertex>  vertices;
    std::vector<TriangleRef> triangles;
};

struct VertexKey {
    int v, vt, vn;
    bool operator==(const VertexKey& other) const {
        return v == other.v && vt == other.vt && vn == other.vn;
    }
};

struct VertexKeyHash {
    size_t operator()(const VertexKey& k) const {
        size_t h = std::hash<int>()(k.v);
        h ^= std::hash<int>()(k.vt) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(k.vn) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

inline MeshLoadResult load_obj(const std::string& path) {
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path, config)) {
        throw std::runtime_error(
            "Failed to load OBJ '" + path + "': " + reader.Error()
        );
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
    bool has_normals = !attrib.normals.empty();

    MeshLoadResult result;
    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> unique_verts;

    for (const tinyobj::shape_t& shape : shapes) {
        size_t offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            uint32_t tri[3];
            for (size_t v = 0; v < 3; v++) {
                tinyobj::index_t idx = shape.mesh.indices[offset + v];
                VertexKey key{ 
                    idx.vertex_index, 
                    idx.texcoord_index, 
                    idx.normal_index 
                };

                auto it = unique_verts.find(key);
                if (it != unique_verts.end()) {
                    tri[v] = it->second;
                    continue;
                }

                MeshVertex mv{};
                mv.position = Vector3(
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                );
                mv.normal = idx.normal_index >= 0
                    ? Vector3(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2])
                    : Vector3(0.0f);
                mv.uv = idx.texcoord_index >= 0
                    ? Vector2(attrib.texcoords[2 * idx.texcoord_index + 0],
                              attrib.texcoords[2 * idx.texcoord_index + 1])
                    : Vector2(0.0f, 0.0f);
                mv.tangent = Vector3(0.0f);

                uint32_t new_idx = (uint32_t)result.vertices.size();
                result.vertices.push_back(mv);
                unique_verts[key] = new_idx;
                tri[v] = new_idx;
            }
            result.triangles.push_back({ tri[0], tri[1], tri[2] });
            offset += 3;
        }
    }

    if (!has_normals) {
        for (const TriangleRef& t : result.triangles) {
            Vector3 face_n = normalize(cross(
                result.vertices[t.v1].position-result.vertices[t.v0].position,
                result.vertices[t.v2].position-result.vertices[t.v0].position
            ));
            result.vertices[t.v0].normal += face_n;
            result.vertices[t.v1].normal += face_n;
            result.vertices[t.v2].normal += face_n;
        }
        for (MeshVertex& v : result.vertices)
            v.normal = v.normal.near_zero() ? 
                Vector3(0.0f, 1.0f, 0.0f) : normalize(v.normal);
    }

    for (const TriangleRef& t : result.triangles) {
        MeshVertex& v0 = result.vertices[t.v0];
        MeshVertex& v1 = result.vertices[t.v1];
        MeshVertex& v2 = result.vertices[t.v2];

        Vector3 e1 = v1.position - v0.position;
        Vector3 e2 = v2.position - v0.position;
        Vector2 duv1 = v1.uv - v0.uv;
        Vector2 duv2 = v2.uv - v0.uv;

        float denom = duv1.x() * duv2.y() - duv2.x() * duv1.y();
        Vector3 tangent = fabsf(denom) < FMIN
            ? Vector3(1.0f, 0.0f, 0.0f)
            : (e1 * duv2.y() - e2 * duv1.y()) * (1.0f / denom);

        v0.tangent += tangent; v1.tangent += tangent; v2.tangent += tangent;
    }
    for (MeshVertex& v : result.vertices) {
        Vector3 t = v.tangent - v.normal * dot(v.tangent, v.normal);
        v.tangent = t.near_zero() ? Vector3(1, 0, 0) : normalize(t);
    }

    return result;
}

// ============================================================================
// MESH CLASS
// ============================================================================

class Mesh : public Hittable {
public:

    __host__ Mesh() : Hittable(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f)) {
        host_data = new HostData();
    }

    __host__ Mesh(
        const Vector3& position,
        const Vector3& rotation,
        const Vector3& scale
    ) : Hittable(position, rotation, scale) {
        host_data = new HostData();
    }

    __host__ Mesh(const Material& material) 
        : Mesh(Vector3(0.0f), Vector3(0.0f), Vector3(1.0f), material) { }

    __host__ Mesh(
        const Vector3&  position,
        const Vector3&  rotation,
        const Vector3&  scale, 
        const Material& material
    ) : Hittable(position, rotation, scale, material) {
        host_data = new HostData();
    }

    __device__ Mesh(
        Transform&    xform, 
        Transform&    delta, 
        size_t        material_index,
        MeshVertex*   d_vertices,
        BVHNode*      d_tri_nodes,
        TriangleRef*  d_triangles
    ) : Hittable(xform, delta, material_index),
        vertices_ptr(d_vertices),
        tri_nodes(d_tri_nodes),
        triangles_ptr(d_triangles)
    { } 

    __host__ static Mesh from_obj(
        const std::string& path,
        const Vector3&  position, 
        const Vector3& rotation, 
        const Vector3& scale,
        const Material& material
    ) {
        Mesh mesh(position, rotation, scale, material);
        MeshLoadResult loaded = load_obj(path);
        mesh.set_geometry(
            std::move(loaded.vertices), 
            std::move(loaded.triangles)
        );
        return mesh;
    }

    __host__ void set_geometry(
        std::vector<MeshVertex> verts,
        std::vector<TriangleRef> tris
    ) {
        host_data->vertices  = std::move(verts);
        host_data->triangles = std::move(tris);
    }

    __host__ Hittable* build() const override {
        auto& vertices  = host_data->vertices;
        auto& triangles = host_data->triangles;

        BVH<TriangleRef> tri_bvh;
        tri_bvh.build(triangles, [&](const TriangleRef& t) {
            return triangle_bbox(
                vertices[t.v0].position, 
                vertices[t.v1].position, 
                vertices[t.v2].position
            );
        });

        MeshVertex*  d_verts;
        BVHNode*     d_nodes;
        TriangleRef* d_tris;

        cudaMalloc(&d_verts, vertices.size() * sizeof(MeshVertex));
        cudaMemcpy(
            d_verts, vertices.data(), 
            vertices.size() * sizeof(MeshVertex), 
            cudaMemcpyHostToDevice
        );

        cudaMalloc(&d_nodes, tri_bvh.nodes.size() * sizeof(BVHNode));
        cudaMemcpy(
            d_nodes, tri_bvh.nodes.data(), 
            tri_bvh.nodes.size() * sizeof(BVHNode), 
            cudaMemcpyHostToDevice
        );

        cudaMalloc(&d_tris, tri_bvh.items.size() * sizeof(TriangleRef));
        cudaMemcpy(
            d_tris, tri_bvh.items.data(), 
            tri_bvh.items.size() * sizeof(TriangleRef), 
            cudaMemcpyHostToDevice
        );

        return device_build<Mesh>(
            xform, delta, material_index, d_verts, d_nodes, d_tris
        );
    }

    __host__ const AABB build_bbox() const override {
        const auto& vertices = host_data->vertices;
        if (vertices.empty()) return AABB();

        Vector3 mn(FMAX, FMAX, FMAX), mx(-FMAX, -FMAX, -FMAX);
        for (const MeshVertex& v : vertices) {
            mn = Vector3(
                fminf(mn.x(), v.position.x()), 
                fminf(mn.y(), v.position.y()), 
                fminf(mn.z(), v.position.z())
            );
            mx = Vector3(
                fmaxf(mx.x(), v.position.x()), 
                fmaxf(mx.y(), v.position.y()), 
                fmaxf(mx.z(), v.position.z())
            );
        }

        Vector3 corners[8] = {
            Vector3(mn.x(), mn.y(), mn.z()), Vector3(mx.x(), mn.y(), mn.z()),
            Vector3(mn.x(), mx.y(), mn.z()), Vector3(mx.x(), mx.y(), mn.z()),
            Vector3(mn.x(), mn.y(), mx.z()), Vector3(mx.x(), mn.y(), mx.z()),
            Vector3(mn.x(), mx.y(), mx.z()), Vector3(mx.x(), mx.y(), mx.z()),
        };

        Vector3 wmn(FMAX, FMAX, FMAX), wmx(-FMAX, -FMAX, -FMAX);
        for (const Vector3& c : corners) {
            Vector3 world = xform.R() * (c * xform.scale()) + xform.p();
            wmn = Vector3(
                fminf(wmn.x(), world.x()), 
                fminf(wmn.y(), world.y()), 
                fminf(wmn.z(), world.z())
            );
            wmx = Vector3(
                fmaxf(wmx.x(), world.x()), 
                fmaxf(wmx.y(), world.y()), 
                fmaxf(wmx.z(), world.z())
            );
        }
        return AABB(wmn, wmx).buffer();
    }

    __device__ bool hit(
        const Ray& ray, 
        Interval   ray_t,
        HitRecord& rec
    ) const override {
        uint32_t stack[TRI_STACK_SIZE];
        uint32_t stack_ptr = 0u;
        stack[stack_ptr++] = 0u;

        bool hit_anything = false;

        while (stack_ptr > 0) {
            uint32_t idx = stack[--stack_ptr];
            const BVHNode& node = tri_nodes[idx];

            if (!node.bbox.hit(ray, ray_t)) continue;

            if (node.object != -1) {
                const TriangleRef& tri = triangles_ptr[node.object];
                const MeshVertex& v0 = vertices_ptr[tri.v0];
                const MeshVertex& v1 = vertices_ptr[tri.v1];
                const MeshVertex& v2 = vertices_ptr[tri.v2];

                float t, u, v;
                bool hit_triangle = intersect_triangle(
                    ray, v0.position, v1.position, v2.position, ray_t, t, u, v
                );

                if (hit_triangle) {
                    hit_anything = true;
                    ray_t.max = t;
                    rec.t = t;
                    rec.p = ray.at(t);
                    interpolate_hit(rec, v0, v1, v2, u, v);
                }
            } else {
                stack[stack_ptr++] = node.left;
                stack[stack_ptr++] = node.right;
            }
        }
        return hit_anything;
    }

    __device__ float pdf_value(
        const Vector3& origin, 
        const Vector3& direction
    ) const override { return 0.0f; }

    __device__ Vector3 random(
        const Vector3& origin, 
        Generator& gen
    ) const override { return Vector3(0.0f, 0.0f, 0.0f); }

private:

    static constexpr uint8_t TRI_STACK_SIZE = 64u;

    struct HostData {
        std::vector<MeshVertex>  vertices;
        std::vector<TriangleRef> triangles;
    };
    HostData* host_data = nullptr;

    MeshVertex*  vertices_ptr  = nullptr;
    BVHNode*     tri_nodes     = nullptr;
    TriangleRef* triangles_ptr = nullptr;

    __host__ static AABB triangle_bbox(
        const Vector3& a, 
        const Vector3& b, 
        const Vector3& c
    ) {
        Vector3 mn(
            fminf(a.x(), fminf(b.x(), c.x())), 
            fminf(a.y(), fminf(b.y(), c.y())), 
            fminf(a.z(), fminf(b.z(), c.z()))
        );
        Vector3 mx(
            fmaxf(a.x(), fmaxf(b.x(), c.x())), 
            fmaxf(a.y(), fmaxf(b.y(), c.y())), 
            fmaxf(a.z(), fmaxf(b.z(), c.z()))
        );
        return AABB(mn, mx);
    }

    __device__ bool intersect_triangle(
        const Ray& ray,
        const Vector3& v0, const Vector3& v1, const Vector3& v2,
        Interval ray_t,
        float& out_t, float& out_u, float& out_v
    ) const {
        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 h = cross(ray.direction(), edge2);
        float a = dot(edge1, h);

        if (fabsf(a) < FMIN) return false;

        float f = 1.0f / a;
        Vector3 s = ray.origin() - v0;
        float u = f * dot(s, h);
        if (u < 0.0f || u > 1.0f) return false;

        Vector3 q = cross(s, edge1);
        float v = f * dot(ray.direction(), q);
        if (v < 0.0f || u + v > 1.0f) return false;

        float t = f * dot(edge2, q);
        if (!ray_t.contains(t)) return false;

        out_t = t; out_u = u; out_v = v;
        return true;
    }

    __device__ void interpolate_hit(
        HitRecord& rec, 
        const MeshVertex& v0, 
        const MeshVertex& v1, 
        const MeshVertex& v2,
        float u, float v
    ) const {
        float w = 1.0f - u - v;
        rec.uv = v0.uv * w + v1.uv * u + v2.uv * v;
        rec.n = normalize(
            v0.normal * w + v1.normal * u + v2.normal * v
        );
        rec.tangent = normalize(
            v0.tangent * w + v1.tangent * u + v2.tangent * v
        );
        
    }
};


