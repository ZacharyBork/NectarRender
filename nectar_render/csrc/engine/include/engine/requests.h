#pragma once

#include <atomic>

class EngineRequests {
public:

    void start()       { start_.store(true, relaxed);       }
    void stop()        { stop_.store(true, relaxed);        }
    void restart()     { restart_.store(true, relaxed);     }
    void shutdown()    { shutdown_.store(true, relaxed);    }
    void rebuild_bvh() { rebuild_bvh_.store(true, relaxed); }


    bool start_pending()     { return start_.exchange(false, relaxed);       }
    bool stop_pending()      { return stop_.exchange(false, relaxed);        }
    bool restart_pending()   { return restart_.exchange(false, relaxed);     }
    bool shutdown_pending()  { return shutdown_.exchange(false, relaxed);    }
    bool bvh_build_pending() { return rebuild_bvh_.exchange(false, relaxed); }

private:

    static constexpr auto relaxed = std::memory_order_relaxed;

    std::atomic<bool> start_       { false };
    std::atomic<bool> stop_        { false };
    std::atomic<bool> restart_     { false };
    std::atomic<bool> shutdown_    { false };
    std::atomic<bool> rebuild_bvh_ { false };

};
