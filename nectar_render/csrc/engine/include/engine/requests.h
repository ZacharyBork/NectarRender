#pragma once

#include <atomic>
#include "core/include/core.h"

class EngineRequests {
public:

    void start()    { start_.store(true, relaxed);    }
    void stop()     { stop_.store(true, relaxed);     }
    void shutdown() { shutdown_.store(true, relaxed); }
    void restart()  { restart_.store(true, relaxed);  }

    bool start_pending()    { return start_.exchange(false, relaxed);    }
    bool stop_pending()     { return stop_.exchange(false, relaxed);     }
    bool restart_pending()  { return restart_.exchange(false, relaxed);  }
    bool shutdown_pending() { return shutdown_.exchange(false, relaxed); }

private:

    std::atomic<bool> start_    { false };
    std::atomic<bool> stop_     { false };
    std::atomic<bool> restart_  { false };
    std::atomic<bool> shutdown_ { false };

};
