#pragma once

#include <cuda_runtime.h>
#include <vector>

#include "engine/include/engine/data.h"
#include "hittable/include/hittable/hittable.h"
#include "engine/include/engine/camera.h"

void trace(
    const HittablesList world,
    Camera       camera,
    DataObject   data, 
    unsigned int max_depth,
    unsigned int seed, 
    unsigned int frame
);
