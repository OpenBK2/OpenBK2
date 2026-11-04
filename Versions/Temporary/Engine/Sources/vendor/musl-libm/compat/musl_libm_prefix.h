/* Force-included ahead of every musl source file (/FI on MSVC, -include on gcc
   and clang), so that the copied files are compiled unmodified.

   It does two things.

   1. Pins the floating point model the code was written for, and refuses to
      build under any other. Determinism across Windows and Linux rests on both
      compilers evaluating every float and double expression in its own type,
      one IEEE operation at a time.

   2. Renames every function and table musl defines to musl_*, public and
      internal alike. The system math.h goes first, under its real names, and
      its include guard keeps it from being read again; only after it are the
      names redefined, so the macros touch musl's code and nothing of the
      platform's. Without it the names collide: glibc's math.h declares
      __sin, __cos and __tan as aliases of sin, cos and tan, and musl uses
      the same names for its argument-reduction kernels. It also keeps
      compilers from recognising the functions as builtins they may evaluate
      at compile time with a different (correctly rounded) library. */

#ifndef MUSL_LIBM_PREFIX_H
#define MUSL_LIBM_PREFIX_H

#define _USE_MATH_DEFINES /* MSVC declares M_PI_2 and friends only on request */
#define _DEFAULT_SOURCE   /* glibc hides them in strict C99 */
#include <float.h>
#include <math.h>
#include "features.h"

/* Excess precision (x87 on 32-bit gcc, FLT_EVAL_METHOD 2) or /fp:fast
   (FLT_EVAL_METHOD -1) would change the results. 32-bit Linux builds need
   -msse2 -mfpmath=sse to get here. */
#if !defined( FLT_EVAL_METHOD ) || FLT_EVAL_METHOD != 0
#error musl-libm needs FLT_EVAL_METHOD 0: SSE2 arithmetic, no /fp:fast
#endif

/* gcc and clang define __FP_FAST_FMA whenever the target has an FMA
   instruction (-march=haswell and later), and log.c, pow.c and log_data.h then
   switch to a different algorithm built on fma(). MSVC never defines it, so a
   Linux build for a newer CPU would compute log and pow differently from every
   Windows build. One algorithm everywhere: the non-FMA one. */
#undef __FP_FAST_FMA

/* Public functions. */
#define sin musl_sin
#define cos musl_cos
#define tan musl_tan
#define asin musl_asin
#define acos musl_acos
#define atan musl_atan
#define atan2 musl_atan2
#define exp musl_exp
#define expm1 musl_expm1
#define expm1f musl_expm1f
#define log musl_log
#define pow musl_pow
#define sinf musl_sinf
#define cosf musl_cosf
#define tanf musl_tanf
#define asinf musl_asinf
#define acosf musl_acosf
#define atanf musl_atanf
#define atan2f musl_atan2f
#define expf musl_expf
#define logf musl_logf
#define powf musl_powf

/* Internal kernels and error helpers, everything libm.h declares. */
#define __rem_pio2_large musl___rem_pio2_large
#define __rem_pio2 musl___rem_pio2
#define __sin musl___sin
#define __cos musl___cos
#define __tan musl___tan
#define __expo2 musl___expo2
#define __rem_pio2f musl___rem_pio2f
#define __sindf musl___sindf
#define __cosdf musl___cosdf
#define __tandf musl___tandf
#define __expo2f musl___expo2f
#define __rem_pio2l musl___rem_pio2l
#define __sinl musl___sinl
#define __cosl musl___cosl
#define __tanl musl___tanl
#define __polevll musl___polevll
#define __p1evll musl___p1evll
#define __signgam musl___signgam
#define __lgamma_r musl___lgamma_r
#define __lgammaf_r musl___lgammaf_r
#define __math_xflowf musl___math_xflowf
#define __math_uflowf musl___math_uflowf
#define __math_oflowf musl___math_oflowf
#define __math_divzerof musl___math_divzerof
#define __math_invalidf musl___math_invalidf
#define __math_xflow musl___math_xflow
#define __math_uflow musl___math_uflow
#define __math_oflow musl___math_oflow
#define __math_divzero musl___math_divzero
#define __math_invalid musl___math_invalid
#define __math_invalidl musl___math_invalidl

/* Lookup tables, from the *_data.h headers. */
#define __exp_data musl___exp_data
#define __exp2f_data musl___exp2f_data
#define __log_data musl___log_data
#define __logf_data musl___logf_data
#define __pow_log_data musl___pow_log_data
#define __powf_log2_data musl___powf_log2_data

/* math.h declared the public functions under their old names, so the renamed
   ones need declaring for the calls musl makes between them (atan2f calls
   atanf, for one). */
#include "../include/musl_libm.h"

#endif
