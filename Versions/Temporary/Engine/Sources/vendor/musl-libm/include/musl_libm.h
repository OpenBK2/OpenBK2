/* The libm functions the simulation uses, from the copy of musl in this
   directory, under names no compiler or system library knows.

   Same source on every platform, compiled by each platform's own compiler
   under the rules compat/musl_libm_prefix.h enforces, so the same input gives
   the same bits on Windows and Linux. The system libm does not: MSVC's CRT and
   glibc round differently on a fraction of inputs, and glibc picks an FMA or
   non-FMA variant by CPU at run time. Accuracy is musl's, within 1 ulp; what
   matters for lockstep is that it is identical everywhere.

   sqrt, fabs, floor and fmod are exact in every CRT and are not here. */

#ifndef MUSL_LIBM_H
#define MUSL_LIBM_H

#ifdef __cplusplus
extern "C"
{
#endif

double musl_sin( double );
double musl_cos( double );
double musl_tan( double );
double musl_asin( double );
double musl_acos( double );
double musl_atan( double );
double musl_atan2( double, double );
double musl_exp( double );
double musl_expm1( double );
double musl_log( double );
double musl_pow( double, double );

float musl_sinf( float );
float musl_cosf( float );
float musl_tanf( float );
float musl_asinf( float );
float musl_acosf( float );
float musl_atanf( float );
float musl_atan2f( float, float );
float musl_expf( float );
float musl_expm1f( float );
float musl_logf( float );
float musl_powf( float, float );

#ifdef __cplusplus
}
#endif

#endif
