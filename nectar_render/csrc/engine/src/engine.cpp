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
    sample_aovs(RenderLayers(&aovs)),
    scene_interface(SceneInterface(&cam, &transfer_stream, &requests_))
{ 
    cudaDeviceSetLimit(cudaLimitStackSize, CUDA_STACK_SIZE_LIMIT);
    reset();
    transfer_stream.link(aovs.get_layer(LayerType::BEAUTY));
    transfer_stream.start();
}

// ============================================================================
// ENGINE STATES
// ============================================================================

RenderReturnState RenderEngine::render() {
    for (uint32_t s = sample_idx; s < n_samples(); s++) {
        scene_interface.update();
        poll_gui_updates();

        if (requests_.restart_pending())
            return RenderReturnState::RESTARTED;

        if (requests_.stop_pending())
            return RenderReturnState::STOPPED;

        sample(sample_idx); sample_idx++;
        with_gil_scoped_acquire(on_frame_finished, sample_idx);
    }
    
    return RenderReturnState::FINISHED;
}

void RenderEngine::idle() {
    set_state(EngineState::IDLE);
    while (!requests_.shutdown_pending()) {

        if (requests_.start_pending()) {                
            reset(); set_state(ES::RENDERING);
            with_gil_scoped_acquire(on_render_started);
            
            transfer_stream.unfreeze();
            RenderReturnState return_state = render();
            cudaDeviceSynchronize(); 
            
            switch (return_state) {
                case RenderReturnState::STOPPED:
                    set_state(ES::IDLE); 
                    with_gil_scoped_acquire(on_stopped); 
                    continue;
                case RenderReturnState::RESTARTED:
                    requests_.start();
                    with_gil_scoped_acquire(on_restarted);
                    continue;
                case RenderReturnState::FINISHED:
                    set_state(ES::IDLE);
                    with_gil_scoped_acquire(on_render_finished);
                    continue;
            }
            
        }

        scene_interface.update();
        std::this_thread::sleep_for(poll_interval);
    }

    with_gil_scoped_acquire(on_shutdown);
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderEngine::set_scene(Scene input_scene) { 
    current_scene = std::move(input_scene);
    current_scene.build();
    scene_interface.update_scene(&current_scene);
}

void RenderEngine::set_trace_mode(TraceMode mode) {
    if (is_rendering()) requests_.stop();
    config.mode = mode;
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

// ============================================================================
// PRIVATE
// ============================================================================

__host__ void RenderEngine::set_state(EngineState s) {
    state.store(s, relaxed);
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

void RenderEngine::poll_gui_updates() {
    auto now = Time::now();
    bool should_poll = now - last_poll_time >= poll_interval;
    
    if (should_poll) {
        last_poll_time = now;
        
        EnginePollResponse response;
        { py::gil_scoped_acquire acquire; response = poll_updates(); }
    
        if (response.should_reset()) {
            cudaDeviceSynchronize();
            if (response.should_update_camera) {
                cam.update(response.camera_params);
                render_mode.store(RenderMode::INTERACTIVE, relaxed);
            }
            requests_.restart();
        }
    } else {
        render_mode.store(RenderMode::FULL, relaxed);
    }
}

void RenderEngine::reset() {
    transfer_stream.freeze();
    cam.__construct(seed);
    
    config.H = (size_t)cam.resolution()[0];
    config.W = (size_t)cam.resolution()[1];
    config.seed = seed;

    sample_idx = 1u;
    aovs.clear();

    if (scene()->is_pending_update()) scene()->update();
    with_gil_scoped_acquire(on_reset);
}

