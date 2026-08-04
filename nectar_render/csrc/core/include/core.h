#pragma once

// COMMON INCLUDES ============================================================

#include <stdint.h>
#include <cuda_runtime.h>

// PACKAGE INCLUDES ===========================================================

#include "core/constants.h"
#include "core/cuda.h"
#include "core/time.h"
#include "core/interval.h"
#include "core/matrix.h"
#include "core/noise.h"
#include "core/onb.h"
#include "core/random.h"
#include "core/utils.h"
#include "core/vector.h"
#include "core/transform.h"

// GLOBAL DEFINES =============================================================

#define NOINLINE __attribute__((noinline))

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::duration<float> fsec;
inline constexpr std::memory_order relaxed = std::memory_order_relaxed;

// UTILITIES ==================================================================

/* Template used to define general no-op for std::function hook defaults meant
 * to be bound via pybind.
 */

template<typename... Args> 
inline void hook_no_op(Args&&... args) {};







