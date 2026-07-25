#pragma once

// COMMON INCLUDES ============================================================

#include <stdint.h>
#include <cuda_runtime.h>

// PACKAGE INCLUDES ===========================================================

#include "core/constants.h"
#include "core/device.h"
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

// UTILITIES ==================================================================

/* Template used to define general no-op for std::function hook defaults meant
 * to be bound via pybind.
 */

template<typename... Args> 
inline void hook_no_op(Args&&... args) {};







