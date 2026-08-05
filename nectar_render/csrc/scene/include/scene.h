#pragma once

#include <vector>
#include <atomic>
#include <cuda_runtime.h>

#include "graph.h"

#include "light/include/skylight.h"
#include "hittable/include/bvh/node.h"
#include "hittable/include/hittable/hittable.h"
#include "hittable/include/hittable/registry.h"
#include "material/include/material/registry.h"

class Scene {
public:

    SceneGraph* graph = nullptr;
    SceneGraph  h_graph{};
    Skylight    skylight;

    HittablesRegistry hittables_registry;
    MaterialRegisty   material_registry;

    // CONSTRUCTORS ===========================================================

    ~Scene() { teardown(); }

    Scene() { }

    Scene(
        std::vector<Hittable*> hittables,
        std::vector<Hittable*> lights,
        Skylight               skylight
    ) : skylight(std::move(skylight)) { 
        hittables.insert(hittables.end(), lights.begin(), lights.end());
        hittables_registry.register_hittables(hittables);
        material_registry.register_materials(hittables_registry.objects());
    }

    Scene(const Scene&) = delete;
    Scene(Scene&& other) noexcept;
    Scene& operator=(const Scene&) = delete;
    Scene& operator=(Scene&& other) noexcept;

    // BUILD / TEARDOWN =======================================================

    void teardown();
    void build();

    // REQUESTS ===============================================================

    void request_reset(
        const bool rebuild_hittables = false,
        const bool rebuild_materials = false,
        const bool update_skylight   = false
    );

    bool is_pending_update();

    // UPDATING ===============================================================

    void add_hittable(std::unique_ptr<Hittable> hittable);

    void build_hittables_registry();
    void build_materials_registry();
    void update();

    // OUTLINE ================================================================

private:

    std::atomic<bool> materials_build_pending { false };
    std::atomic<bool> hittables_build_pending { false };
    std::atomic<bool> skylight_update_pending { false };

};

