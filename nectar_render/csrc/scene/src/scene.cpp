#include "scene/include/scene.h"

// ============================================================================
// SCENE CLASS METHODS
// ============================================================================

Scene::Scene(Scene&& other) noexcept
  : skylight(std::move(other.skylight)),
    hittables_registry(std::move(other.hittables_registry)),
    material_registry(std::move(other.material_registry)),
    graph(other.graph),
    h_graph(other.h_graph)
{
    materials_build_pending.store(
        other.materials_build_pending.load(relaxed), 
        relaxed
    );
    hittables_build_pending.store(
        other.hittables_build_pending.load(relaxed), 
        relaxed
    );
    skylight_update_pending.store(
        other.skylight_update_pending.load(relaxed), 
        relaxed
    );
    other.graph = nullptr;
}

Scene& Scene::operator=(Scene&& other) noexcept {
    if (this != &other) {
        skylight = std::move(other.skylight);
        hittables_registry = std::move(other.hittables_registry);
        material_registry = std::move(other.material_registry);
        graph = other.graph;
        h_graph = other.h_graph;
        materials_build_pending.store(
            other.materials_build_pending.load(relaxed), 
            relaxed
        );
        hittables_build_pending.store(
            other.hittables_build_pending.load(relaxed), 
            relaxed
        );
        skylight_update_pending.store(
            other.skylight_update_pending.load(relaxed), 
            relaxed
        );
        other.graph = nullptr;
    }
    return *this;
}

// BUILD / TEARDOWN ===========================================================

void Scene::teardown() {
    if (!graph) return;
    CUDAMemory::free(graph); 

    hittables_registry.destroy_device_hittables();
    material_registry.teardown();

    CUDAMemory::free(h_graph.lights);

    graph   = nullptr;
    h_graph = SceneGraph();
}

void Scene::build() {
    build_materials_registry();
    build_hittables_registry();

    h_graph.skylight = skylight.build();

    CUDAMemory::allocate_if_null<SceneGraph>(graph);
    CUDAMemory::copy<SceneGraph>(graph, &h_graph);
}

// REQUESTS ===================================================================

void Scene::request_reset(
    const bool rebuild_hittables,
    const bool rebuild_materials,
    const bool update_skylight
) {
    if (rebuild_hittables)
        hittables_build_pending.store(true, relaxed);
    if (rebuild_materials)
        materials_build_pending.store(true, relaxed);
    if (update_skylight)
        skylight_update_pending.store(true, relaxed);
}

bool Scene::is_pending_update() {
    return materials_build_pending.load(relaxed)
        || hittables_build_pending.load(relaxed)
        || skylight_update_pending.load(relaxed);
}

// UPDATING ===================================================================

void Scene::add_hittable(std::unique_ptr<Hittable> hittable) {
    material_registry.register_material(hittable.get());
    hittables_registry.register_hittable(std::move(hittable));
}

void Scene::build_materials_registry() {
    material_registry.destroy_device_materials();
    material_registry.build_device_materials();
    h_graph.materials = material_registry.device_materials();
}

void Scene::build_hittables_registry() {
    hittables_registry.build();
    h_graph.objects   = hittables_registry.device_hittables();
    h_graph.bvh_nodes = hittables_registry.device_bvh_nodes();
    h_graph.lights    = hittables_registry.device_lights();
}

void Scene::update() {
    if (materials_build_pending.exchange(false, relaxed))
        build_materials_registry();

    if (hittables_build_pending.exchange(false, relaxed))
        build_hittables_registry();

    if (skylight_update_pending.exchange(false, relaxed))
        h_graph.skylight = skylight.build();

    CUDAMemory::allocate_if_null<SceneGraph>(graph);
    CUDAMemory::copy<SceneGraph>(graph, &h_graph);
}
