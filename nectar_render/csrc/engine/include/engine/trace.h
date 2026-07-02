#pragma once

#include <cuda_runtime.h>

#include "engine/include/engine/data.h"
#include "engine/include/engine/camera.h"
#include "engine/include/engine/scene.h"
#include "engine/include/engine/light.h"

void trace(
    Camera        camera,
    Scene&        scene,
    RenderLayers& layers, 
    unsigned int  max_depth,
    unsigned int  seed, 
    unsigned int  frame
);
