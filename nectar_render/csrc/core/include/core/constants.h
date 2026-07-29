#pragma once

// ############################################################################
// NUMERICAL CONSTANTS
// ############################################################################

inline constexpr float EPS  = 1e-4f;
inline constexpr float INF  = __FLT_MAX__;
inline constexpr float FMIN = 1e-8f;
inline constexpr float FMAX = 1e32f;
inline constexpr float PI   = 3.1415926535897932385f;
inline constexpr float PI2  = PI * 2.0f;
inline constexpr float PI4  = PI2 * PI2;

// ############################################################################
// CUDA CONSTANTS
// ############################################################################

#define CUDA_STACK_SIZE_LIMIT 2048 // Memory limit for per-thread call stack.

#define BS1D 256u // 1-dimensional block size for CUDA allocation.
#define BS2D 16u  // 2-dimensional block size for CUDA allocation.

