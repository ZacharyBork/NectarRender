#include "engine/include/engine/engine.h"
#include "engine/include/engine/trace.h"

#include "core/include/core/utils.h"
#include "core/include/core/random.h"

void RenderEngine::initialize(
    std::vector<size_t> _output_shape,
    unsigned int seed
) {
    output_shape = _output_shape;
    color        = DataObject(output_shape);
    random_seed  = seed;
    initialized  = true;
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
        std::make_shared<Sphere>(Point3(0,0,-1), 0.5),
        std::make_shared<Sphere>(Point3(0,-100.5,-1), 100)
    };
    build_scene(world);

    frame_idx++;
    trace(scene, color, random_seed, frame_idx);
    return reinterpret_cast<uintptr_t>(color.device_ptr);
}


