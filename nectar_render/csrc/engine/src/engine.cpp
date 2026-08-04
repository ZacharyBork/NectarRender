#include "engine/include/engine/engine.h"

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
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
    cam.__construct(seed);
    transfer_stream.link(aovs.get_layer(LayerType::BEAUTY));
    transfer_stream.start();
}

RenderEngine::~RenderEngine() { aovs.free_aovs(); }

// ============================================================================
// ENGINE STATES
// ============================================================================

RenderReturnState RenderEngine::render() {
    for (uint32_t s = sample_idx; s < n_samples(); s++) {
        scene_interface.update();
        poll_gui_updates();

        if (requests_.stop_pending())
            return RenderReturnState::STOPPED;

        uint32_t depth = is_interactive() ? 2u : ray_depth;
        trace_full(
            cam, current_scene.graph, sample_aovs.aovs(), 
            sample_idx, depth, seed
        );

        aovs.accumulate(sample_aovs, sample_idx);
        sample_aovs.clear();
        
        cudaDeviceSynchronize(); sample_idx++;
        with_gil_scoped_acquire(on_frame_finished, sample_idx);

        if (requests_.restart_pending())
            return RenderReturnState::RESTARTED;
    }
    
    return RenderReturnState::FINISHED;
}

RenderReturnState RenderEngine::viewport() {
    scene_interface.update();
    poll_gui_updates();

    if (requests_.stop_pending())
        return RenderReturnState::STOPPED;

    trace_viewport(cam, current_scene.graph, sample_aovs.aovs(), seed);

    aovs.accumulate(sample_aovs, 1u);
    sample_aovs.clear();
    cudaDeviceSynchronize();
    with_gil_scoped_acquire(on_frame_finished, sample_idx);
    
    if (requests_.restart_pending())
        return RenderReturnState::RESTARTED;

    return RenderReturnState::WAITING;
}

void RenderEngine::idle() {
    set_state(EngineState::IDLE);
    while (!requests_.shutdown_pending()) {

        if (requests_.start_pending()) {                
            reset(); set_state(ES::RENDERING);
            with_gil_scoped_acquire(on_render_started);
            
            RenderReturnState return_state = (
                engine_type.load(relaxed) == EngineType::VIEWPORT
            ) ? viewport() : render();
            
            switch (return_state) {
                case RenderReturnState::WAITING: requests_.start(); break;
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
            if (engine_type.load(relaxed) == EngineType::PATHTRACER)
                requests_.restart();
        }
    } else if (render_mode.load(relaxed) == RenderMode::INTERACTIVE) {
        render_mode.store(RenderMode::FULL, relaxed);
    }
}

void RenderEngine::reset() {
    sample_idx = 1u;
    if (scene()->is_pending_update()) scene()->update();
    with_gil_scoped_acquire(on_reset);
}

