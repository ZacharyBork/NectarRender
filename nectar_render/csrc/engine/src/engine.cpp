#include "engine/include/engine/engine.h"
#include "engine/include/engine/trace.h"

#include "material/include/material/material.h"
#include "engine/include/engine/denoise.h"

void RenderEngine::initialize(
    const Camera& camera,
    unsigned int samples   = 10,
    unsigned int ray_depth = 8,
    unsigned int seed      = 54321
) {
    cam.emplace(camera);
    render_layers.emplace(RenderLayers(cam->resolution));
    num_samples = samples;
    max_depth   = ray_depth;
    random_seed = seed;
    initialized = true;
}

void RenderEngine::render(Scene& scene) {
    if (!initialized) {
        throw std::runtime_error(
            "RenderEngine::render called before initialization."
        );
    }

    RenderLayers rl(layers());
    render_layers->pin_buffer(LayerType::BEAUTY);
    
    for (int sample = 0; sample < num_samples; sample++) {
        frame_idx++;

        rl.clear();
        trace(scene, *cam, rl, max_depth, random_seed, frame_idx);
        render_layers->combine(rl);

        cuda_synchronize();
        on_frame_finished(frame_idx);
    }

    render_layers->normalize_by_samples(num_samples);
}

RenderLayers* RenderEngine::layers() { return &render_layers.value(); }


