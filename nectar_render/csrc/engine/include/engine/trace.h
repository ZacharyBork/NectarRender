#pragma once

#include <cuda_runtime.h>
#include <vector>

#include "engine/include/engine/data.h"
#include "hittable/include/hittable/hittable.h"

void trace(
    const HittablesList world,
    DataObject   data, 
    unsigned int seed, 
    unsigned int frame
);
