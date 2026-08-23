#pragma once

// Simple static detection of possible vector SIMD instruction sets, given by the compiler
#if defined(__SSE2__) || defined(_M_X64) || ( defined(_M_IX86_FP) && _M_IX86_FP >= 2 )
#define HAS_SSE2 1
#pragma message("SSE2 is supported!")
#else
#define HAS_SSE2 0
#endif

#if defined(__AVX2__)
#pragma message("AVX2 is supported!")
#define HAS_AVX2 1
#else
#define HAS_AVX2 0
#endif

#if defined(__AVX512F__)
#pragma message("AVX512F is supported!")
#define HAS_AVX512F 1
#else
#define HAS_AVX512F 0
#endif
