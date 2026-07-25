#pragma once

#include <mutex>
#include <chrono>
#include <atomic>
#include <optional>
#include <pybind11/functional.h>

#include "core/include/core.h"
#include "host/include/host/utils.h"
#include "hittable/include/hittable/hittable.h"
#include "interface/include/scene_interface.h"

#include "data/include/data.h"
#include "scene.h"
#include "camera.h"
#include "trace.h"

// ============================================================================
// GLOBALS
// ============================================================================

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<float> fsec;
inline constexpr std::memory_order relaxed = std::memory_order_relaxed;

// ============================================================================
// DATA UTILS
// ============================================================================

enum class EngineState{ IDLE, RENDERING }; typedef EngineState ES;
enum class RenderMode { FULL, INTERACTIVE };

// ============================================================================
// GUI POLLING
// ============================================================================

struct EnginePollResponse {    
    bool should_update_camera = false;
    CameraParams camera_params;

    bool should_reset() const {
        return should_update_camera;
    }
};

static inline EnginePollResponse default_poll_response = {};
inline EnginePollResponse poll_updates_default() { 
    return default_poll_response; 
}

// ============================================================================
// RENDER ENGINE CLASS
// ============================================================================

class RenderEngine {
public:

    std::function<void()>         on_render_started  = hook_no_op<>;
    std::function<void(uint32_t)> on_frame_finished  = hook_no_op<uint32_t>;
    std::function<void()>         on_render_finished = hook_no_op<>;
    std::function<void()>         on_stopped         = hook_no_op<>;
    std::function<void()>         on_restarted       = hook_no_op<>;
    std::function<void()>         on_reset           = hook_no_op<>;
    std::function<void()>         on_shutdown        = hook_no_op<>;

    std::function<EnginePollResponse()> poll_updates = poll_updates_default;

    /* CONSTRUCTION */

    ~RenderEngine() { current_scene.teardown(); }

    RenderEngine(
        const Camera& camera,
        uint32_t ray_depth = 8u,
        uint32_t seed      = 54321u
    );

    /* PROPERTY ACCESS */

    Scene*          scene()  { return &current_scene; }
    Camera*         camera() { return &cam; }
    RenderLayers*   layers() { return &aovs; }
    TransferStream* stream() { return &transfer_stream; }

    /* ENGINE STATE */

    EngineState get_state() const { return state.load(relaxed); }

    bool is_rendering() const { return get_state()==ES::RENDERING; }
    bool is_idle()      const { return get_state()==ES::IDLE;      }

    void request_start()    { start_requested.store(true, relaxed);    }
    void request_stop()     { stop_requested.store(true, relaxed);     }
    void request_restart()  { restart_requested.store(true, relaxed);  }
    void request_shutdown() { shutdown_requested.store(true, relaxed); }

    /* RENDER MODE */

    RenderMode get_render_mode() const    { return render_mode.load(relaxed); }
    void set_render_mode(RenderMode mode) { render_mode.store(mode, relaxed); }

    bool is_interactive() const { 
        return get_render_mode() == RenderMode::INTERACTIVE; 
    }

    /* FUNCTION QUEUE */

    void queue_function(
        std::function<void()> func, 
        bool rebuild_bvh = false, 
        bool immediate = true
    );

    /* RENDERING */

    void set_scene(Scene input_scene);

    void render() {
        for (uint32_t s = sample_idx; s < n_samples(); s++) {
            if (poll_gui_updates()) return;

            if (stop_requested.exchange(false, relaxed)) {
                with_gil_scoped_acquire(on_stopped);
                return;
            }

            if (restart_requested.exchange(false, relaxed)) {
                start_requested.store(true, relaxed);
                with_gil_scoped_acquire(on_restarted);
                return;
            }

            sample(sample_idx);
            with_gil_scoped_acquire(on_frame_finished, sample_idx);
            sample_idx++;
        }
    }

    void idle() {
        set_state(EngineState::IDLE);
        while (!shutdown_requested.load(relaxed)) {

            if (start_requested.exchange(false, relaxed)) {                
                reset(); set_state(ES::RENDERING);
                with_gil_scoped_acquire(on_render_started);
                
                render(); cudaDeviceSynchronize(); 
                
                set_state(ES::IDLE);
                with_gil_scoped_acquire(on_render_finished);
                continue;
            }

            std::this_thread::sleep_for(poll_interval);
        }

        with_gil_scoped_acquire(on_shutdown);
    }

    /* UTILITIES */

    const uint32_t n_samples() const;
    void set_n_samples(uint32_t n);

    const uint32_t max_depth() const;
    void set_max_depth(uint32_t value);

    /* SCENE INTERFACE UTILS */

    SceneInterface& get_scene_interface();
    void screen_space_ray(float u, float v);

private:

    Camera       cam;
    TraceConfig  config;
    RenderLayers aovs, sample_aovs;
    uint32_t     ray_depth, seed;

    uint32_t sample_idx = 1u;

    Scene          current_scene;
    TransferStream transfer_stream;
    SceneInterface scene_interface;

    std::atomic<bool> start_requested    { false };
    std::atomic<bool> stop_requested     { false };
    std::atomic<bool> restart_requested  { false };
    std::atomic<bool> shutdown_requested { false };
    std::atomic<bool> bvh_build_pending  { false };

    std::atomic<EngineState> state       { EngineState::IDLE };
    std::atomic<RenderMode>  render_mode { RenderMode::FULL  };

    Time::time_point last_poll_time{};
    static constexpr auto poll_interval = std::chrono::milliseconds(16);

    std::mutex function_queue_mutex;
    std::vector<std::function<void()>> function_queue{};


    void set_state(EngineState s);
    void process_function_queue();
    void sample(uint32_t sample_index = 1u);

    bool poll_gui_updates() {
        auto now = Time::now();
        bool should_poll = now - last_poll_time >= poll_interval;
        bool should_reset = false;
        
        if (should_poll) {
            last_poll_time = now;
            EnginePollResponse response;
            { py::gil_scoped_acquire acquire; response = poll_updates(); }
        
            if (response.should_reset()) {
                
                cudaDeviceSynchronize();
                if (response.should_update_camera) {
                    cam.parameters_()->update(response.camera_params);
                    scene_interface.update(cam.device_camera());
                    render_mode.store(RenderMode::INTERACTIVE, relaxed);
                }

                start_requested.store(true, relaxed); 
                should_reset = true;
            }
        } else {
            render_mode.store(RenderMode::FULL, relaxed);
        }
        return should_reset;
    }

    void reset() {
        process_function_queue();

        cam.__construct(seed);
        
        config.H = (size_t)cam.resolution()[0];
        config.W = (size_t)cam.resolution()[1];
        config.seed = seed;

        sample_idx = 1u;
        aovs.clear();

        if (bvh_build_pending.load(relaxed)) {
            bvh_build_pending.store(false, relaxed);
            scene()->build();
        }

        with_gil_scoped_acquire(on_reset);
    }

};
