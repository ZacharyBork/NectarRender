#pragma once

// ############################################################################
// NUMERICAL CONSTANTS
// ############################################################################

const float EPS  = (float)1e-3;
const float INF  = __FLT_MAX__;
const float FMIN = (float)1e-4;
const float FMAX = (float)1e32;
const float PI   = 3.1415926535897932385;

// ############################################################################
// CUDA CONSTANTS
// ############################################################################

#define BS2D 16 // 2-dimensional block size for CUDA allocation.

