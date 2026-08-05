#pragma once

#include "scene/include/scene.h"

inline constexpr size_t MAX_OUTLINER_ITEMS = 512UL;

struct SceneNode {
    size_t object_id;
    size_t parent_id;
    size_t material_id;

    bool is_light;

    std::string name;
    std::string type_name;

};

struct SceneOutline {
    std::vector<SceneNode> nodes;

    SceneOutline() {
        nodes.reserve(MAX_OUTLINER_ITEMS);
    }

    void reset() {
        nodes.clear();
    }

    void add_node(SceneNode node) {
        if (nodes.size() >= MAX_OUTLINER_ITEMS)
            throw std::runtime_error(
                "SceneOutline exceeded maximum node count [" 
                + std::to_string(MAX_OUTLINER_ITEMS) + "]."
            );
            
        nodes.push_back(node);
    }
};

class SceneOutliner {
public:

    SceneOutliner() { }
    void set_scene(Scene* new_scene) { scene = new_scene; }

    SceneOutline get_outline() const { return outline; }

    void build_outline() {
        outline.reset();
        std::vector<HittableRegistryEntry*> hittables = (
            scene->hittables_registry.get_all_entries()
        );
        
        for (HittableRegistryEntry* obj : hittables) {
            SceneNode node;
            node.object_id   = obj->object_id;
            node.parent_id   = obj->parent_id;
            node.material_id = obj->material_id;
            node.is_light    = obj->is_light;
            node.name        = obj->name;
            node.type_name   = hittabletype_to_string(obj->type);
            outline.add_node(node);
        }
    }

private:

    Scene* scene = nullptr;
    SceneOutline outline;

};

