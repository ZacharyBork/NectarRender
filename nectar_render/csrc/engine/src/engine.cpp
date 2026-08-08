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
    transfer_stream.link_denoise_aux(
        aovs.get_layer(LayerType::DIFFUSE),
        aovs.get_layer(LayerType::WORLD_NORMAL)
    );

    // transfer_stream.link(aovs.get_layer(LayerType::WORLD_NORMAL));


    
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

        // uint32_t depth = is_interactive() ? 2u : ray_depth;
        uint32_t depth = ray_depth;
        trace_full(
            cam, current_scene.graph, sample_aovs.aovs(), 
            TraceConfig{ 
                sample_idx, depth, seed, true, 3u, 
                is_interactive() ? 0.15f : 0.95f
            }
        );

        transfer_stream.guarded([&](){ 
            aovs.accumulate(sample_aovs, sample_idx);
        });
        
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

    sample_aovs.clear();
    trace_viewport(
        cam, current_scene.graph, sample_aovs.aovs(), 
        show_axis_grid.load(relaxed), seed
    );
    transfer_stream.guarded([&](){ 
        aovs.overwrite(sample_aovs);
    });
    
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
            
            handle_render_return_state(return_state);
            continue;
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

void RenderEngine::update_streaming_layer(LayerType layer) {
    transfer_stream.link(aovs.get_layer(layer));
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

void RenderEngine::set_axis_grid_visible(const bool visible) {
    show_axis_grid.store(visible, relaxed);
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

void RenderEngine::handle_render_return_state(RenderReturnState state) {
    switch (state) {
        case RenderReturnState::WAITING:
            cudaDeviceSynchronize(); requests_.start();
            std::this_thread::sleep_for(viewport_refresh_interval);    
            break;
        case RenderReturnState::STOPPED:
            set_state(ES::IDLE); 
            with_gil_scoped_acquire(on_stopped); 
            break;
        case RenderReturnState::RESTARTED:
            requests_.start();
            with_gil_scoped_acquire(on_restarted);
            break;
        case RenderReturnState::FINISHED:
            set_state(ES::IDLE);
            with_gil_scoped_acquire(on_render_finished);
            break;
    }
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
    } else if (render_mode.load(relaxed) == RenderMode::INTERACTIVE) {
        render_mode.store(RenderMode::FULL, relaxed);
    }
}

void RenderEngine::reset() {
    sample_idx = 1u;
    if (scene()->is_pending_update()) scene()->update();
    with_gil_scoped_acquire(on_reset);
}

