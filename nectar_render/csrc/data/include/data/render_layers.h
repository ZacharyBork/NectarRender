#pragma once

#include "data/include/data/data_object.h"

// ============================================================================
// RENDER LAYERS CLASS
// ============================================================================

const inline uint8_t N_RENDER_LAYERS = 8;

enum class LayerType {
    BEAUTY, DIFFUSE, SPECULAR, NORMAL, SHADOW, DEPTH, EMISSION, OBJECT_ID
};

struct RenderLayersConfig {
    bool beauty    = true;
    bool diffuse   = false;
    bool specular  = false;
    bool normal    = false;
    bool shadow    = false;
    bool depth     = false;
    bool emission  = false;
    bool object_id = false;
};

struct AOVs {
    size_t H, W;
    DataView beauty, diffuse, specular, normal, 
             shadow, depth, emission, object_id;

    __host__ AOVs(
        size_t H, size_t W,
        DataObject& beauty,
        DataObject& diffuse,
        DataObject& specular,
        DataObject& normal,
        DataObject& shadow,
        DataObject& depth,
        DataObject& emission,
        DataObject& object_id
    ) : H(H), W(W),
        beauty(beauty.view()), 
        diffuse(diffuse.view()), 
        specular(specular.view()),
        normal(normal.view()), 
        shadow(shadow.view()), 
        depth(depth.view()), 
        emission(emission.view()),
        object_id(object_id.view()) 
    { }
};

class RenderLayers {
public:

    size_t H, W;
    RenderLayersConfig cfg;

    DataObject beauty;
    DataObject diffuse;
    DataObject specular;
    DataObject normal;
    DataObject shadow;
    DataObject depth;
    DataObject emission;
    DataObject object_id;

    __host__ RenderLayers() : H((size_t)0), W((size_t)0), cfg({}) { }

    __host__ RenderLayers(
        size_t h, 
        size_t w,
        const RenderLayersConfig& cfg = {}
    ) : H((size_t)h), W((size_t)w), cfg(cfg) { 
        build_layers(); 
    }

    __host__ RenderLayers(
        const Vector2& res,
        const RenderLayersConfig& cfg = {}
    ) : RenderLayers(res[0], res[1], cfg) { }

    __host__ RenderLayers(
        RenderLayers* t,
        const RenderLayersConfig& cfg = {}
    ) : RenderLayers(t->H, t->W, t->cfg) { }

    __host__ AOVs* aovs() {
        AOVs aovs_obj(
            H, W, beauty, diffuse, specular, normal, 
            shadow, depth, emission, object_id
        );

        AOVs* d_aov_ptr;
        size_t n_bytes = sizeof(aovs_obj);
        cudaMalloc(&d_aov_ptr, n_bytes);
        cudaMemcpy(
            reinterpret_cast<void*>(d_aov_ptr), 
            &aovs_obj, n_bytes, cudaMemcpyHostToDevice);

        return d_aov_ptr;

    }

    __host__ std::array<DataObject*, N_RENDER_LAYERS> get_data() {
        return {
            &beauty, &diffuse, &specular, &normal,
            &shadow, &depth, &emission, &object_id
        };
    }

    __host__ void combine(RenderLayers& other) { 
        std::array<DataObject*, N_RENDER_LAYERS> this_data = get_data();
        std::array<DataObject*, N_RENDER_LAYERS> other_data = other.get_data();
        for (int i = 0; i < N_RENDER_LAYERS; i++) {
            this_data[i]->combine(*other_data[i]);
        }    
    }

    __host__ void accumulate(
        RenderLayers& other,
        uint32_t current_sample
    ) {
        std::array<DataObject*, N_RENDER_LAYERS> this_data = get_data();
        std::array<DataObject*, N_RENDER_LAYERS> other_data = other.get_data();
        for (int i = 0; i < N_RENDER_LAYERS; i++) {
            this_data[i]->accumulate_samples(*other_data[i], current_sample);
        }
    }

    __host__ void clear() {
        for (DataObject* obj : get_data())
            if (obj->is_enabled())
                cudaMemset(obj->data_ptr(), 0, obj->n_bytes());
    }

    __host__ void normalize_by_samples(uint32_t total_samples) {
        for (DataObject* obj : get_data())
            if (obj->is_enabled()) obj->normalize_by_samples(total_samples);
    }

    __host__ DataObject* get_layer(LayerType layer_type) {
        switch (layer_type) {
        case LayerType::BEAUTY:    return &beauty;
        case LayerType::DIFFUSE:   return &diffuse;
        case LayerType::SPECULAR:  return &specular;
        case LayerType::NORMAL:    return &normal;
        case LayerType::SHADOW:    return &shadow;
        case LayerType::DEPTH:     return &depth;
        case LayerType::EMISSION:  return &emission;
        case LayerType::OBJECT_ID: return &object_id;
        default:
            throw std::runtime_error(
                "DataObject::get_layer() encountered invalid LayerType.");
        }
    }

private:

    __host__ DataObject construct_data_object(size_t channels) {
        return DataObject(channels, H, W);
    }

    __host__ void build_layers() {
        if (cfg.beauty)    beauty    = construct_data_object(3);
        if (cfg.diffuse)   diffuse   = construct_data_object(3);
        if (cfg.specular)  specular  = construct_data_object(3);
        if (cfg.normal)    normal    = construct_data_object(3);
        if (cfg.shadow)    shadow    = construct_data_object(1);
        if (cfg.depth)     depth     = construct_data_object(1);
        if (cfg.emission)  emission  = construct_data_object(3);
        if (cfg.object_id) object_id = construct_data_object(3);
    }

};


