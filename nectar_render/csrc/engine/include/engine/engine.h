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
#include "requests.h"

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
enum class RenderReturnState{ FINISHED, STOPPED, RESTARTED };

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

    RenderEngine(
        const Camera& camera,
        uint32_t ray_depth = 8u,
        uint32_t seed      = 54321u
    );

    ~RenderEngine() { current_scene.teardown(); }

    RenderEngine(const RenderEngine&) = delete;
    RenderEngine& operator=(const RenderEngine&) = delete;
    RenderEngine(RenderEngine&&) = delete;
    RenderEngine& operator=(RenderEngine&&) = delete;

    /* PROPERTY ACCESS */

    Scene*          scene()    { return &current_scene;   }
    Camera*         camera()   { return &cam;             }
    RenderLayers*   layers()   { return &aovs;            }
    TransferStream* stream()   { return &transfer_stream; }
    EngineRequests* requests() { return &req;             }

    /* ENGINE STATE */

    EngineState get_state() const { return state.load(relaxed); }
    bool is_rendering() const { return get_state()==ES::RENDERING; }
    bool is_idle()      const { return get_state()==ES::IDLE;      }

    /* RENDER MODE */

    RenderMode get_render_mode() const    { return render_mode.load(relaxed); }
    void set_render_mode(RenderMode mode) { render_mode.store(mode, relaxed); }

    bool is_interactive() const { 
        return get_render_mode() == RenderMode::INTERACTIVE; 
    }

    /* ENGINE STATES */

    RenderReturnState render();

    void idle();

    /* UTILITIES */

    void set_scene(Scene input_scene);
    SceneInterface& get_scene_interface();

    const uint32_t n_samples() const;
    void set_n_samples(uint32_t n);

    const uint32_t max_depth() const;
    void set_max_depth(uint32_t value);

private:

    Camera       cam;
    TraceConfig  config;
    RenderLayers aovs, sample_aovs;
    uint32_t     ray_depth, seed;

    uint32_t sample_idx = 1u;

    Scene          current_scene;
    TransferStream transfer_stream;
    SceneInterface scene_interface;

    EngineRequests req;
    std::atomic<EngineState> state       { EngineState::IDLE };
    std::atomic<RenderMode>  render_mode { RenderMode::FULL  };

    Time::time_point last_poll_time{};
    static constexpr auto poll_interval = std::chrono::milliseconds(16);

    void set_state(EngineState s);
    void sample(uint32_t sample_index = 1u);
    void poll_gui_updates();
    void reset();

};
