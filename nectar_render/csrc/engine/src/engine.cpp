#include "engine/include/engine/engine.h"
#include "engine/include/engine/trace.h"

#include "core/include/core/utils.h"
#include "core/include/core/random.h"

void RenderEngine::initialize(
    const CameraParams& camera_params,
    unsigned int samples   = 10,
    unsigned int ray_depth = 8,
    unsigned int seed      = 54321
) {
    cam_params.emplace(camera_params);
    render_layers.emplace(
        RenderLayers(
            cam_params->resolution[1], 
            cam_params->resolution[0]
        )
    );
    num_samples = samples;
    max_depth   = ray_depth;
    random_seed = seed;
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

    Camera camera(*cam_params);

    std::vector<std::shared_ptr<Hittable>> world = {
        std::make_shared<Sphere>(Vector3(0.0f,    0.0f, -1.0f),   0.5f),
        std::make_shared<Sphere>(Vector3(0.0f, -100.5f, -1.0f), 100.0f)
    };
    build_scene(world);

    for (int sample = 0; sample < num_samples; sample++) {
        frame_idx++;

        DataObject color(&render_layers->beauty);
        trace(scene, camera, color, max_depth, random_seed, frame_idx);
        render_layers->beauty.combine(color);
    }

    render_layers->normalize_by_samples(num_samples);
    return reinterpret_cast<uintptr_t>(render_layers->beauty.device_ptr);
}


