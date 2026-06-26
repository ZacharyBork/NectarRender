#pragma once

#include <cuda_runtime.h>

template<typename T, typename... Args>
T* device_build(Args... args);

#include "core/include/core/constants.h"
#include "core/include/core/interval.h"
#include "core/include/core/matrix.h"
#include "core/include/core/random.h"
#include "core/include/core/utils.h"
#include "core/include/core/vector.h"



