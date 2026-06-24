#include "engine/include/engine/engine.h"
#include "engine/include/engine/trace.h"

#include "core/include/core/utils.h"
#include "core/include/core/random.h"

#include "material/include/material/material.h"

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
    const std::vector<Hittable*>& host_scene
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

    Lambertian mat1 = Lambertian(Color(1.0f, 0.5f, 0.2f));
    Lambertian mat2 = Lambertian(Color(0.8f, 0.8f, 0.8f));

    Sphere s1(Vector3(0.0f,    0.0f, -1.0f),   0.5f, mat1.build());
    Sphere s2(Vector3(0.0f, -100.5f, -1.0f), 100.0f, mat2.build());
    build_scene({ &s1, &s2 });

    for (int sample = 0; sample < num_samples; sample++) {
        float prog = (float)(sample+1) / (float)num_samples;
        on_sample(sample+1, num_samples, prog);
        frame_idx++;

        DataObject color(&render_layers->beauty);
        trace(scene, camera, color, max_depth, random_seed, frame_idx);
        render_layers->beauty.combine(color);
    }

    render_layers->normalize_by_samples(num_samples);
    render_layers->beauty.linear_to_gamma();
    return reinterpret_cast<uintptr_t>(render_layers->beauty.device_ptr);
}


