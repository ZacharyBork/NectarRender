#pragma once

#include <cuda_runtime.h>
#include <vector>

#include "engine/include/engine/data.h"

void trace(DataObject data, unsigned int seed, unsigned int frame);
