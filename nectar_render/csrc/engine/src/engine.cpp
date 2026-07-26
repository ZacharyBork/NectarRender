#include "engine/include/engine/engine.h"



// ============================================================================
// CONSTRUCTORS
// ============================================================================

RenderEngine::RenderEngine(
    const Camera& camera,
    uint32_t ray_depth,
    uint32_t seed
) : cam(camera),
    ray_depth(ray_depth),
    seed(seed),
    aovs(RenderLayers(cam.resolution())),
    sample_aovs(RenderLayers(&aovs))
{ 
    reset();
    transfer_stream.link(aovs.get_layer(LayerType::BEAUTY));
    transfer_stream.start();
}

// ============================================================================
// ENGINE STATE
// ============================================================================



// ============================================================================
// FUNCTION QUEUE
// ============================================================================

void RenderEngine::queue_function(
    std::function<void()> func, 
    bool rebuild_bvh, 
    bool immediate
) {
    { 
        std::lock_guard<std::mutex> lock(function_queue_mutex); 
        function_queue.push_back(func); 
    }
    if (rebuild_bvh)
        bvh_build_pending.store(true, relaxed);
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderEngine::set_scene(Scene input_scene) { 
    current_scene = std::move(input_scene); 
}

// ============================================================================
// UTILITIES
// ============================================================================

const uint32_t RenderEngine::n_samples() const { return cam.n_samples(); }
void RenderEngine::set_n_samples(uint32_t n) {
    cudaDeviceSynchronize();
    cam.parameters_()->samples_per_pixel = n;
    reset();
}

const uint32_t RenderEngine::max_depth() const { return ray_depth; }
void RenderEngine::set_max_depth(uint32_t value) {
    cudaDeviceSynchronize();
    ray_depth = value;
    reset();
}

// ============================================================================
// SCENE INTERFACE UTILS
// ============================================================================

SceneInterface& RenderEngine::get_scene_interface() { return scene_interface; }

void RenderEngine::screen_space_ray(float u, float v) {
    HitRecord* d_rec;
    CUDAMemory::allocate<HitRecord>(d_rec);
    hit_test_ray(u, v, current_scene.graph, cam.device_camera(), d_rec);

    HitRecord rec;
    cudaMemcpy(&rec, d_rec, sizeof(HitRecord), cudaMemcpyDeviceToHost);
    CUDAMemory::free<HitRecord>(d_rec);

    scene_interface.configure(&current_scene, &transfer_stream, rec);
}

// ============================================================================
// PRIVATE
// ============================================================================

__host__ void RenderEngine::set_state(EngineState s) {
    state.store(s, relaxed);
}

__host__ void RenderEngine::process_function_queue() {
    std::vector<std::function<void()>> to_run;
    { 
        std::lock_guard<std::mutex> lock(function_queue_mutex); 
        to_run.swap(function_queue); 
    }
    if (to_run.empty()) return;

    {
        py::gil_scoped_acquire acquire;
        for (auto& func : to_run) func();
        to_run.clear();
    }
}

void RenderEngine::sample(uint32_t sample_index) {
    uint32_t depth = is_interactive() ? 2u : ray_depth;
    trace(
        config, cam.device_camera(), current_scene.graph, 
        sample_aovs.aovs(), sample_index, depth
    );

    aovs.accumulate(sample_aovs, sample_index);
    sample_aovs.clear();
    cudaDeviceSynchronize();
}

