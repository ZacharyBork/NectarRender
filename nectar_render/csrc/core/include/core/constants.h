#pragma once

// ############################################################################
// NUMERICAL CONSTANTS
// ############################################################################

const float EPS  = 1e-3f;
const float INF  = __FLT_MAX__;
const float FMIN = 1e-4f;
const float FMAX = 1e32f;
const float PI   = 3.1415926535897932385f;
const float PI2  = PI * 2.0f;

// ############################################################################
// CUDA CONSTANTS
// ############################################################################

#define BS1D 256 // 1-dimensional block size for CUDA allocation.
#define BS2D 16u // 2-dimensional block size for CUDA allocation.

