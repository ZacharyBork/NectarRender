#pragma once

#include <cuda_runtime.h>
#include <vector>

#include "core/include/core/constants.h"
#include "engine/include/engine/data.h"
#include "engine/include/engine/camera.h"
#include "hittable/include/hittable/hittable.h"
#include "material/include/material/material.h"

void trace(
    const HittablesList world,
    Camera       camera,
    DataObject   data, 
    unsigned int max_depth,
    unsigned int seed, 
    unsigned int frame
);
