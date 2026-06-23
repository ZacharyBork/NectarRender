#include "engine/include/engine/engine.h"
#include "engine/include/engine/trace.h"

#include "core/include/core/utils.h"
#include "core/include/core/random.h"

void RenderEngine::initialize(
    std::vector<size_t> _output_shape,
    unsigned int seed
) {
    output_shape = _output_shape;
    random_seed  = seed;
    render_layers.emplace(RenderLayers(output_shape));
    
    initialized = true;
}

void RenderEngine::build_scene(
    const std::vector<std::shared_ptr<Hittable>>& host_scene
) {
    int n = host_scene.size();

    std::vector<Hittable*> device_ptrs(n);
    for (int i = 0; i < n; i++) {
        device_ptrs[i] = host_scene[i]->build();
    }

    cudaMalloc(&d_objects, n * sizeof(Hittable*));
    cudaMemcpy(d_objects, device_ptrs.data(),
               n * sizeof(Hittable*),
               cudaMemcpyHostToDevice);

    scene = HittablesList{ d_objects, n };
}

uintptr_t RenderEngine::render() {
    if (!initialized) {
        throw std::runtime_error(
            "RenderEngine::render called before initialization."
        );
    }

    std::vector<std::shared_ptr<Hittable>> world = {
        std::make_shared<Sphere>(Point3(0.0f,    0.0f, -1.0f),   0.5f),
        std::make_shared<Sphere>(Point3(0.0f, -100.5f, -1.0f), 100.0f)
    };
    build_scene(world);

    for (int sample = 0; sample < samples_per_pixel; sample++) {
        frame_idx++;

        DataObject color(output_shape);
        trace(scene, color, random_seed, frame_idx);
        render_layers->beauty.combine(color);
    }

    render_layers->normalize_by_samples(samples_per_pixel);
    return reinterpret_cast<uintptr_t>(render_layers->beauty.device_ptr);
}


