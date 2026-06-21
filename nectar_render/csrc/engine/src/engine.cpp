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

uintptr_t RenderEngine::render() {
    if (!initialized) {
        throw std::runtime_error(
            "RenderEngine::render called before initialization."
        );
    }
    frame_idx++;
    trace(color, random_seed, frame_idx);
    return reinterpret_cast<uintptr_t>(color.device_ptr);
}


