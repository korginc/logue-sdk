/*
    BSD 3-Clause License

    Copyright (c) 2018, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

#ifndef BBAC8CC0_8473_452E_801A_60AD28A72398
#define BBAC8CC0_8473_452E_801A_60AD28A72398

#ifndef A4247454_9632_47F2_AD08_C3E0FB9034E2
#define A4247454_9632_47F2_AD08_C3E0FB9034E2

/**
 * @file    float_math.h
 * @brief   Floating Point Math Utilities.
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_float_math Floating-Point Math
 * @{
 *
 */

#ifndef __float_math_h
#define __float_math_h

#ifdef fmax
#undef fmax
#endif
#ifdef fmin
#undef fmin
#endif
#include <math.h>
#include <stdint.h>
#if defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#endif

// this is stdfloat.h content
#ifdef __cplusplus

using float32_t = float;
using float64_t = float;

#else

typedef float  float32_t;
typedef float float64_t;

#endif // __cplusplus
// end of stdfloat.h content

/*===========================================================================*/
/* Constants.                                                                */
/*===========================================================================*/

/**
 * @name    Constants
 * @{
 */

#ifndef M_E
#define M_E 2.718281828459045f
#endif

#ifndef M_LOG2E
#define M_LOG2E  1.44269504088896f
#endif

#ifndef M_LOG10E
#define M_LOG10E 0.4342944819032518f
#endif

#ifndef M_LN2
#define M_LN2   0.6931471805599453094f
#endif

#ifndef M_LN10
#define M_LN10  2.30258509299404568402f
#endif

#ifndef M_LN100
#define M_LN100 4.60517018598809136804f
#endif

#ifndef M_TWOLN100
#define M_TWOLN100 9.21034037197618273608f
#endif

#ifndef M_PI
#define M_PI    3.141592653589793f
#endif

#ifndef M_TWOPI
#define M_TWOPI 6.283185307179586f
#endif

#ifndef M_PI_2
#define M_PI_2 1.5707963267948966f
#endif

#ifndef M_PI_4
#define M_PI_4 0.7853981633974483f
#endif

#ifndef M_1_PI
#define M_1_PI 0.3183098861837907f
#endif

#ifndef M_2_PI
#define M_2_PI 0.6366197723675814f
#endif

#ifndef M_4_PI
#define M_4_PI 1.2732395447351627f
#endif

#ifndef M_1_TWOPI
#define M_1_TWOPI 0.15915494309189534f
#endif

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.1283791670955126f
#endif

#ifndef M_4_PI2
#define M_4_PI2 0.40528473456935109f
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880f
#endif

#ifndef M_1_SQRT2
#define M_1_SQRT2 0.7071067811865475f
#endif

#ifndef M_LOG2_E
#define M_LOG2_E 1.4426950408889634f
#endif

// ln(2) = 1 / ln(e)
#ifndef M_INV_LOG2_E
#define M_INV_LOG2_E 0.6931471805599453f
#endif

// 20 / ln(10)
#ifndef M_LOGTODB
#define M_LOGTODB 8.6858896380650365530225783783321f
#endif

#ifndef M_DBTOLOG
#define M_DBTOLOG 0.11512925464970228420089957273422f
#endif

// Constants for optimized logarithmic scaling
#define FASTERLOGF_10000      9.204111f
#define FASTERLOGF_50         3.89179f
#define INV_FASTERLOGF_10000  0.10857362f // 1.0f / 9.21034037f (ln(10000))
#define INV_FASTERLOGF_50     0.25562225f // 1.0f / 3.91202301f (ln(50))

/** @} */

/*===========================================================================*/
/* Macros.                                                                   */
/*===========================================================================*/

/**
 * @name    Macros
 * @{
 */

#define F32_FRAC_MASK ((1L<<23)-1)
#define F32_EXP_MASK  (((1L<<9)-1)<<23)
#define F32_SIGN_MASK (0x80000000)

//Note: need to pass an integer representation
#define f32_frac_bits(f) ((f) & F32_FRAC_MASK)
#define f32_exp_bits(f)  ((f) & F32_EXP_MASK)
#define f32_sign_bit(f)  ((f) & F32_SIGN_MASK)

/** @} */

/*===========================================================================*/
/* Types.                                                                    */
/*===========================================================================*/

/**
 * @name    Types
 * @{
 */

typedef union {
  float f;
  uint32_t i;
} f32_t;

typedef struct {
  float a;
  float b;
} f32pair_t;

/** Make a float pair.
 */
static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair(const float a, const float b) {
  return (f32pair_t){a, b};
}

/** @} */

/*===========================================================================*/
/* Operations.                                                               */
/*===========================================================================*/

/**
 * @name    Operations
 * @{
 */

/** FSEL construct
 */
static inline __attribute__((optimize("Ofast"),always_inline))
float fsel(const float a, const float b, const float c) {
  return (a >= 0) ? b : c;
}

/** FSEL boolean construct
 */
static inline __attribute__((optimize("Ofast"),always_inline))
uint8_t fselb(const float a) {
  return (a >= 0) ? 1 : 0;
}

/** Sign bit check.
 */
static inline __attribute__((always_inline))
uint8_t float_is_neg(const f32_t f) {
  return (f.i >> 31) != 0;
}

/** Obtain mantissa
 */
static inline __attribute__((always_inline))
int32_t float_mantissa(f32_t f) {
  return f.i & ((1 << 23) - 1);
}

/** Obtain exponent
 */
static inline __attribute__((always_inline))
int32_t float_exponent(f32_t f) {
  return (f.i >> 23) & 0xFF;
}

/** Pair-wise addition
 */
static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_add(const f32pair_t p0, const f32pair_t p1) {
  return (f32pair_t){p0.a + p1.a, p0.b + p1.b};
}

/** Pair-wise subtraction
 */
static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_sub(const f32pair_t p0, const f32pair_t p1) {
  return (f32pair_t){p0.a - p1.a, p0.b - p1.b};
}

/** Pair-wise scalar addition
 */
static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_addscal(const f32pair_t p, const float scl) {
  return (f32pair_t){p.a + scl, p.b + scl};
}

/** Pair-wise product
 */
static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_mul(const f32pair_t p0, const f32pair_t p1) {
  return (f32pair_t){p0.a * p1.a, p0.b * p1.b};
}

/** Pair-wise scalar product
 */
static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_mulscal(const f32pair_t p, const float scl) {
  return (f32pair_t){p.a * scl, p.b * scl};
}

/** Pair-wise linear interpolation
 */
static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_linint(const float fr, const f32pair_t p0, const f32pair_t p1) {
  const float frinv = 1.f - fr;
  return (f32pair_t){ frinv * p0.a + fr * p1.a, frinv * p0.b + fr * p1.b };
}

/** Return x with sign of y applied
 */
static inline __attribute__((optimize("Ofast"),always_inline))
float si_copysignf(const float x, const float y)
{
  f32_t xs = {x};
  f32_t ys = {y};

  xs.i &= 0x7fffffff;
  xs.i |= ys.i & 0x80000000;

  return xs.f;
}

/** Absolute value
 */
static inline __attribute__((optimize("Ofast"),always_inline))
float si_fabsf(float x)
{
  f32_t xs = {x};
  xs.i &= 0x7fffffff;
  return xs.f;
}

/** Floor function
 */
static inline __attribute__((optimize("Ofast"),always_inline))
float si_floorf(float x)
{
  return (float)((uint32_t)x);
}

/** Ceiling function
 */
static inline __attribute__((optimize("Ofast"),always_inline))
float si_ceilf(float x)
{
  return (float)((uint32_t)x + 1);
}

/** Round to nearest integer.
 */
static inline __attribute__((optimize("Ofast"),always_inline))
float si_roundf(float x)
{
  return (float)((int32_t)(x + si_copysignf(0.5f,x)));
}

static inline __attribute__((optimize("Ofast"), always_inline))
float clampfsel(const float min, float x, const float max)
{
  x = fsel(x - min, x, min);
  return fsel(x - max, max, x);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float clampminfsel(const float min, const float x)
{
  return fsel(x - min, x, min);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float clampmaxfsel(const float x, const float max)
{
  return fsel(x - max, max, x);
}

#if defined(FLOAT_CLIP_NOFSEL)

/** Clip upper bound of x to m (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clipmaxf(const float x, const float m)
{ return (((x)>=m)?m:(x)); }

/** Clip lower bound of x to m (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clipminf(const float  m, const float x)
{ return (((x)<=m)?m:(x)); }

/** Clip x to min and max (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clipminmaxf(const float min, const float x, const float max)
{ return (((x)>=max)?max:((x)<=min)?min:(x)); }

/** Clip lower bound of x to 0.f (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clip0f(const float x)
{ return (((x)<0.f)?0.f:(x)); }

/** Clip upper bound of x to 1.f (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clip1f(const float x)
{ return (((x)>1.f)?1.f:(x)); }

/** Clip x to [0.f, 1.f] (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clip01f(const float x)
{ return (((x)>1.f)?1.f:((x)<0.f)?0.f:(x)); }

/** Clip lower bound of x to -1.f (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clipm1f(const float x)
{ return (((x)<-1.f)?-1.f:(x)); }

/** Clip x to [-1.f, 1.f] (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clip1m1f(const float x)
{ return (((x)>1.f)?1.f:((x)<-1.f)?-1.f:(x)); }

#else

/** Clip upper bound of x to m (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clipmaxf(const float x, const float m)
{ return clampmaxfsel(x, m); }

/** Clip lower bound of x to m (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clipminf(const float  m, const float x)
{ return clampminfsel(m, x); }

/** Clip x to min and max (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clipminmaxf(const float min, const float x, const float max)
{ return clampfsel(min, x, max); }

/** Clip lower bound of x to 0.f (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clip0f(const float x)
{ return clampminfsel(0, x); }

/** Clip upper bound of x to 1.f (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clip1f(const float x)
{ return clampmaxfsel(x, 1); }

/** Clip x to [0.f, 1.f] (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clip01f(const float x)
{ return clampfsel(0, x, 1); }

/** Clip lower bound of x to -1.f (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clipm1f(const float x)
{ return clampminfsel(-1, x); }

/** Clip x to [-1.f, 1.f] (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float clip1m1f(const float x)
{ return clampfsel(-1, x, 1); }

#endif

/** @} */

/*===========================================================================*/
/* Useful Functions.                                                         */
/*===========================================================================*/

/**
 * @name    Faster direct approximations of common trigonometric functions
 * @note    Use with care. Depending on optimizations and targets these can provide little benefit over libc versions.
 * @{
 */

/*=====================================================================*
 *                                                                     *
 *  fastersinf, fastercosf, fastersinfullf, fastercosfullf,            *
 *  fastertanfullf, fasterpow2f, fasterexpf, fasterlog2f,              *
 *  and fasterpowf adapted from FastFloat code with the following      *
 *  disclaimer:                                                        *
 *                                                                     *
 *                                                                     *
 *                   Copyright (C) 2011 Paul Mineiro                   *
 * All rights reserved.                                                *
 *                                                                     *
 * Redistribution and use in source and binary forms, with             *
 * or without modification, are permitted provided that the            *
 * following conditions are met:                                       *
 *                                                                     *
 *     * Redistributions of source code must retain the                *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer.                                       *
 *                                                                     *
 *     * Redistributions in binary form must reproduce the             *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer in the documentation and/or            *
 *     other materials provided with the distribution.                 *
 *                                                                     *
 *     * Neither the name of Paul Mineiro nor the names                *
 *     of other contributors may be used to endorse or promote         *
 *     products derived from this software without specific            *
 *     prior written permission.                                       *
 *                                                                     *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND              *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,         *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES               *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE             *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER               *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,                 *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES            *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE           *
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR                *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY              *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE             *
 * POSSIBILITY OF SUCH DAMAGE.                                         *
 *                                                                     *
 * Contact: Paul Mineiro <paul@mineiro.com>                            *
 *=====================================================================*/

/** "Fast" sine approximation, valid for x in [-M_PI, M_PI]
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastsinf(float x) {
  static const float q = 0.78444488374548933f;
  union { float f; uint32_t i; } p = { 0.20363937680730309f };
  union { float f; uint32_t i; } r = { 0.015124940802184233f };
  union { float f; uint32_t i; } s = { -0.0032225901625579573f };

  union { float f; uint32_t i; } vx = { x };
  uint32_t sign = vx.i & 0x80000000;
  vx.i = vx.i & 0x7FFFFFFF;

  float qpprox = M_4_PI * x - M_4_PI2 * x * vx.f;
  float qpproxsq = qpprox * qpprox;

  p.i |= sign;
  r.i |= sign;
  s.i ^= sign;

  return q * qpprox + qpproxsq * (p.f + qpproxsq * (r.f + qpproxsq * s.f));
}

/** "Faster" sine approximation, valid for x in [-M_PI, M_PI]
 * @note Adapted from Paul Mineiro's FastFloat
 * @note Warning: can be slower than libc version!
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastersinf(float x) {
  static const float q = 0.77633023248007499f;
  union { float f; uint32_t i; } p = { 0.22308510060189463f };
  union { float f; uint32_t i; } vx = { x };
  const uint32_t sign = vx.i & 0x80000000;
  vx.i &= 0x7FFFFFFF;
  const float qpprox = M_4_PI * x - M_4_PI2 * x * vx.f;
  p.i |= sign;
  return fmaxf(-1.0f, fminf(1.0f, qpprox * (q + p.f * qpprox)));
}

/** "Fast" sine approximation, valid on full x domain
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastersinfullf(float x) {
  const int32_t k = (int32_t)(x * M_1_TWOPI);
  const float half = (x < 0) ? -0.5f : 0.5f;
  return fastsinf((half + k) * M_TWOPI - x);
}


/** "Fast" cosine approximation, valid for x in [-M_PI, M_PI]
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastcosf(float x) {
  const float halfpiminustwopi = -4.7123889803846899f;
  float offset = (x > M_PI_2) ? halfpiminustwopi : M_PI_2;
  return fastsinf(x + offset);
}

/** "Faster" cosine approximation, valid for x in [-M_PI, M_PI]
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastercosf(float x) {
  static const float p = 0.54641335845679634f;
  union { float f; uint32_t i; } vx = { x };
  vx.i &= 0x7FFFFFFF;
  const float qpprox = 1.0f - M_2_PI * vx.f;
  return qpprox + p * qpprox * (1.0f - qpprox * qpprox);
}


/** "Faster" cosine approximation, valid on full x domain
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastercosfullf(float x) {
  return fastersinfullf(x + M_PI_2);
}

/** "Fast" tangent approximation, valid for x in [-M_PI_2, M_PI_2]
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasttanf(float x) {
  return fastsinf(x) / fastsinf(x + M_PI_2);
}

/** "Faster" tangent approximation, valid for x in [-M_PI_2, M_PI_2]
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastertanf(float x) {
  return fastcosf(x) / fastercosf(x);
}

/** "Fast" tangent approximation, valid on full x domain, except where tangent diverges.
 * @note Adapted from Paul Mineiro's FastFloat
 * @note Warning: can be slower than libc version!
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasttanfullf(float x) {
  const int32_t k = (int32_t)(x * M_1_TWOPI);
  const float half = (x < 0) ? -0.5f : 0.5f;
  const float xnew = x - (half + k) * M_TWOPI;
  return fastsinf(xnew)/fastcosf(xnew);
}

/** "Faster" tangent approximation, valid on full x domain, except where tangent diverges.
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastertanfullf(float x) {
  const int32_t k = (int32_t)(x * M_1_TWOPI);
  const float half = (x < 0) ? -0.5f : 0.5f;
  const float xnew = x - (half + k) * M_TWOPI;
  return fastersinf(xnew)/fastercosf(xnew);
}

/** "Fast" log base 2 approximation, valid for positive x as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastlog2f(float x) {
  union { float f; uint32_t i; } vx = { x };
  union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
  float y = vx.i;
  y *= 1.1920928955078125e-7f;

  return y - 124.22551499f
           - 1.498030302f * mx.f
           - 1.72587999f / (0.3520887068f + mx.f);
}

/** "Faster" log base 2 approximation, valid for positive x as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterlog2f(float x) {
  union { float f; uint32_t i; } vx = { x };
  float y = (float)(vx.i);
  y *= 1.1920928955078125e-7f;
  return y - 126.94269504f;
}

/** "Fast" natural logarithm (log) approximation, valid for positive x as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastlogf(float x) {
  return M_LN2 * fastlog2f(x);
}

/** "Faster" natural logarithm approximation, valid for positive x as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterlogf(float x) {
  return M_LN2 * fasterlog2f(x);
}

// Real-time Audio (Frequency modulation, envelopes).
static inline float fast_pow2(float p) {
    // 2^p approximation using IEEE-754 floating point trick
    // 8388608.0f = 2^23
    // 1065353216 = 127 << 23 (exponent bias)
    union { float f; int32_t i; } u;
    u.i = (int32_t)(p * 8388608.0f) + 1065353216;
    return u.f;
}


/** "Fast" power of 2 approximation, valid for x in [ -126, ... as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
 * Precision-critical math (e.g. tuning tables).
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastpow2f(float p) {
  float clipp = (p < -126) ? -126.0f : p;
  int w = clipp;
  float z = clipp - w + 1.f;
  union { uint32_t i; float f; } v = { (uint32_t) ( (1 << 23) *
      (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z)
      ) };

  return v.f;
}

/** "Faster" power of 2 approximation, valid for x in [ -126, ... as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
 * General purpose where input might be $-\infty$.
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterpow2f(float p) {
  float clipp = (p < -126) ? -126.0f : p;
  union { uint32_t i; float f; } v = { (uint32_t)( (1 << 23) * (clipp + 126.94269504f) ) };
  return v.f;
}

/** "Fast" x to the power of p approximation
 * @note Adapted from Paul Mineiro's FastFloat
 * @note Warning: Seems to have divergent segments with discontinuities for some base/exponent combinations
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastpowf(float x, float p) {
  return fastpow2f(p * fastlog2f(x));
}

/** "Faster" x to the power of p approximation
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterpowf(float x, float p) {
  return fasterpow2f(p * fasterlog2f(x));
}

/** "Fast" exponential approximation, valid for x in [ ~ -87, ... as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastexpf(float p) {
  return fastpow2f(M_LOG2E * p);
}

/** "Faster" exponential approximation, valid for x in [ ~ -87, ... as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterexpf(float p) {
  return fasterpow2f(M_LOG2E * p);
}

/** "Fast" natural exponential approximation
 * @note Adapted from https://codingforspeed.com/using-faster-exponential-approximation/
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float e_expf(float x) {
  x = 1.0 + x / 256.0;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x; x *= x; x *= x;
  return x;
}

/** More precise natural exponential approximation
 * @note Adapted from https://codingforspeed.com/using-faster-exponential-approximation/
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float e_expff(float x) {
  x = 1.0 + x / 1024;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x; x *= x; x *= x;
  x *= x; x *= x;
  return x;
}

/*= End of FastFloat derived code =====================================*/

/** atan2 approximation
 * @note Adapted from http://dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasteratan2f(float y, float x) {
  const float coeff_1 = M_PI_4;
  const float coeff_2 = 3 * coeff_1;
  float abs_y = si_fabsf(y) + 1e-10f; // kludge to prevent 0/0 condition
  float r, angle;
  if (x >= 0) {
    r = (x - abs_y) / (x + abs_y);
    angle = coeff_1 - coeff_1 * r;
  }
  else {
    r = (x + abs_y) / (abs_y - x);
    angle = coeff_2 - coeff_1 * r;
  }
  return (y < 0) ? -angle : angle; // negate if in quad III or IV
}

/** Hyperbolic tangent approximation
 * @note Adapted from http://math.stackexchange.com/questions/107292/rapid-approximation-of-tanhx
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastertanhf(float x) {
  return (-0.67436811832e-5f +
          (0.2468149110712040f +
           (0.583691066395175e-1f + 0.3357335044280075e-1f * x) * x) * x) /
    (0.2464845986383725f +
     (0.609347197060491e-1f +
      (0.1086202599228572f + 0.2874707922475963e-1f * x) * x) * x);
}

/** @} */

/*===========================================================================*/
/* Useful Conversions.                                                       */
/*===========================================================================*/

/**
 * @name    Useful Conversions
 * @note    These can be very slow, use with caution. Should use table lookups in performance critical sections.
 * @{
 */

/** Amplitude to dB
 * @note Will remove low boundary check in future version
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float ampdbf(const float amp) {
  return (amp < 0.f) ? -999.f : 20.f*log10f(amp);
}

/** "Faster" Amplitude to dB
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterampdbf(const float amp) {
  static const float c = 3.3219280948873626f; // 20.f / log2f(10);
  return c*fasterlog2f(amp);
}

/** dB to ampltitude
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float dbampf(const float db) {
  return powf(10.f,0.05f*db);
}

/** "Faster" dB to ampltitude
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterdbampf(const float db) {
  return fasterpowf(10.f, 0.05f*db);
}

/** @} */

/*===========================================================================*/
/* Interpolation.                                                            */
/*===========================================================================*/

/**
 * @name    Interpolations
 * @todo    Add cubic/spline interpolations
 * @{
 */

/** Linear interpolation
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float linintf(const float fr, const float x0, const float x1) {
  return x0 + fr * (x1 - x0);
}

/** Cosine interpolation
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float cosintf(const float fr, const float x0, const float x1) {
  const float tmp = (1.f - fastercosfullf(fr * M_PI)) * 0.5f;
  return x0 + tmp * (x1 - x0);
}

/** @} */
/*===========================================================================*/
/* Square root approximation.                                                            */
/*===========================================================================*/

/**
* @name    Euclidean distance
* @{
    */

/** Fast square root approximation based on Newton-Raphson Method
 *
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastSqrt(float x) {
    if (x == 0) return 0;
    float approx = x / 2.0f; // Initial guess
    for (int i = 0; i < 5; ++i) { // 5 iterations for good precision
        approx = (approx + x / approx) / 2.0f;
    }
    return approx;
}

/** Faster square root approximation SQRT_Blinn_Modified based on this article:
 *  https://link.springer.com/article/10.1007/s11075-024-01932-7
 *  see github https://github.com/pawelgepner/Heron-Square-Root-Function/blob/main/main.cpp#L429
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterSqrt (float x)
{ union { float x; unsigned i; } u;
    u.x = x;
    u.i = (u.i >> 1) + 0x1fbb4f2e;
    return u.x;
}
/**
 * @brief algorithm for the required 15-bit accuracy is proposed.
 * see doc above
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fasterSqrt_15bits (float x)
{   int i = *(int*) & x;
    i = 0x5f1110a0 - (i >> 1);
    float y = *(float*) & i;
    float c = x * y;
    float d = c * y;
    y = c * (2.2825186f - d * (2.2533049f - d));
    return y;
}


 /** @} */


/*===========================================================================*/
/* Euclidean distance.                                                            */
/*===========================================================================*/

/**
 * @name    Euclidean distance
 * @{
 */


/** Fast but imprecise approximation for square root of
 * qudratic sum
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float sqrtsum2(const float dx, const float dy) {
  return (dy > dx) ? (dy + dx * 0.5) : (dx + dy * 0.5);
}

/** More accurate approximation for square root of
 * qudratic sum
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float sqrtsum2acc(const float dx, const float dy) {
//   return (dy > dx) ? (0.41 * dx + 0.941246 * dy) : (0.941246 * dx + 0.41 * dy);
  return (dy > dx) ? (0.41 * dx + 0.96 * dy) : (0.96 * dx + 0.41 * dy);
}


/** Better approximation for square root of
 * qudratic sum
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float sqrtsum2bett(const float dx, const float dy) {
    // const float _40div1024 = 40 / 1024f;
    // const float _441div1024 = 441 / 1024f;
    // const float _1007div1024 = 1007 / 1024f;
    const float _40div1024 = 40.0 / 1024;
    const float _441div1024 = 441.0 / 1024;
    const float _1007div1024 = 1007.0 / 1024;
    //maxD = max(dx,dy); minD = min(dx,dy):
    float maxD, minD;
    if (dx > dy)
    { maxD = dx; minD = dy; }
    else { maxD = dy; minD = dx; }
    auto term3 = (maxD < 16 * minD) ? (maxD * _40div1024) : 0;
    return (maxD * _1007div1024) + (minD * _441div1024) - term3;
}

/** This the famous Fast inverse square root code from Quake III!
 * https://en.wikipedia.org/wiki/Fast_inverse_square_root
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float Q_rsqrt( float number )
{
  // safety check against zero division
  if (number <= 0.0f) {
    number = 1.0e-9f;
  }

	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;                       // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}


/** Fast but imprecise approximation for the euclidean distance
 * given carthesian coordinates for two points (x1,y1) and (x2,y2)
 * https://en.m.wikibooks.org/wiki/Algorithms/Distance_approximations
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float eucDist1(const float x1, const float y1, const float x2, const float y2) {
    float dx = fabs(x2 - x1);
    float dy = fabs(y2 - y1);
    return sqrtsum2(dx, dy);
}


/** A fast approximation of 2D distance based on an octagonal boundary
 * given carthesian coordinates for two points (x1,y1) and (x2,y2)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float eucDist2(const float x1, const float y1, const float x2, const float y2) {
    float dx = fabs(x2 - x1);
    float dy = fabs(y2 - y1);
    return sqrtsum2acc(dx, dy);
}


/** A better approximation for the euclidean distance
 * given carthesian coordinates for two points (x1,y1) and (x2,y2)
 * https://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float eucDist2Bett(const float x1, const float y1, const float x2, const float y2) {
    float dx = fabs(x2 - x1);
    float dy = fabs(y2 - y1);
    return sqrtsum2bett(dx, dy);
}

/* NEON implementation of sin, cos, exp and log

   Inspired by Intel Approximate Math library, and based on the
   corresponding algorithms of the cephes math library
*/

/* Copyright (C) 2011  Julien Pommier

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)
*/


typedef float32x4_t v4sf;  // vector of 4 float
typedef uint32x4_t v4su;  // vector of 4 uint32
typedef int32x4_t v4si;  // vector of 4 uint32

#define c_inv_mant_mask ~0x7f800000u
#define c_cephes_SQRTHF 0.707106781186547524
#define c_cephes_log_p0 7.0376836292E-2
#define c_cephes_log_p1 - 1.1514610310E-1
#define c_cephes_log_p2 1.1676998740E-1
#define c_cephes_log_p3 - 1.2420140846E-1
#define c_cephes_log_p4 + 1.4249322787E-1
#define c_cephes_log_p5 - 1.6668057665E-1
#define c_cephes_log_p6 + 2.0000714765E-1
#define c_cephes_log_p7 - 2.4999993993E-1
#define c_cephes_log_p8 + 3.3333331174E-1
#define c_cephes_log_q1 -2.12194440e-4
#define c_cephes_log_q2 0.693359375

/* natural logarithm computed for 4 simultaneous float
   return NaN for x <= 0
*/
static inline __attribute__((optimize("Ofast"), always_inline))
v4sf log_ps(v4sf x) {
  v4sf one = vdupq_n_f32(1);

  x = vmaxq_f32(x, vdupq_n_f32(0)); /* force flush to zero on denormal values */
  v4su invalid_mask = vcleq_f32(x, vdupq_n_f32(0));

  v4si ux = vreinterpretq_s32_f32(x);

  v4si emm0 = vshrq_n_s32(ux, 23);

  /* keep only the fractional part */
  ux = vandq_s32(ux, vdupq_n_s32(c_inv_mant_mask));
  ux = vorrq_s32(ux, vreinterpretq_s32_f32(vdupq_n_f32(0.5f)));
  x = vreinterpretq_f32_s32(ux);

  emm0 = vsubq_s32(emm0, vdupq_n_s32(0x7f));
  v4sf e = vcvtq_f32_s32(emm0);

  e = vaddq_f32(e, one);

  /* part2:
     if( x < SQRTHF ) {
       e -= 1;
       x = x + x - 1.0;
     } else { x = x - 1.0; }
  */
  v4su mask = vcltq_f32(x, vdupq_n_f32(c_cephes_SQRTHF));
  v4sf tmp = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(x), mask));
  x = vsubq_f32(x, one);
  e = vsubq_f32(e, vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(one), mask)));
  x = vaddq_f32(x, tmp);

  v4sf z = vmulq_f32(x,x);

  v4sf y = vdupq_n_f32(c_cephes_log_p0);
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p1));
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p2));
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p3));
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p4));
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p5));
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p6));
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p7));
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, vdupq_n_f32(c_cephes_log_p8));
  y = vmulq_f32(y, x);

  y = vmulq_f32(y, z);


  tmp = vmulq_f32(e, vdupq_n_f32(c_cephes_log_q1));
  y = vaddq_f32(y, tmp);


  tmp = vmulq_f32(z, vdupq_n_f32(0.5f));
  y = vsubq_f32(y, tmp);

  tmp = vmulq_f32(e, vdupq_n_f32(c_cephes_log_q2));
  x = vaddq_f32(x, y);
  x = vaddq_f32(x, tmp);
  x = vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(x), invalid_mask)); // negative arg will be NAN
  return x;
}

#define c_exp_hi 88.3762626647949f
#define c_exp_lo -88.3762626647949f

#define c_cephes_LOG2EF 1.44269504088896341
#define c_cephes_exp_C1 0.693359375
#define c_cephes_exp_C2 -2.12194440e-4

#define c_cephes_exp_p0 1.9875691500E-4
#define c_cephes_exp_p1 1.3981999507E-3
#define c_cephes_exp_p2 8.3334519073E-3
#define c_cephes_exp_p3 4.1665795894E-2
#define c_cephes_exp_p4 1.6666665459E-1
#define c_cephes_exp_p5 5.0000001201E-1

/* exp() computed for 4 float at once */
static inline __attribute__((optimize("Ofast"), always_inline)) v4sf
exp_ps(v4sf x) {
  v4sf tmp, fx;

  v4sf one = vdupq_n_f32(1);
  x = vminq_f32(x, vdupq_n_f32(c_exp_hi));
  x = vmaxq_f32(x, vdupq_n_f32(c_exp_lo));

  /* express exp(x) as exp(g + n*log(2)) */
  fx = vmlaq_f32(vdupq_n_f32(0.5f), x, vdupq_n_f32(c_cephes_LOG2EF));

  /* perform a floorf */
  tmp = vcvtq_f32_s32(vcvtq_s32_f32(fx));

  /* if greater, substract 1 */
  v4su mask = vcgtq_f32(tmp, fx);
  mask = vandq_u32(mask, vreinterpretq_u32_f32(one));


  fx = vsubq_f32(tmp, vreinterpretq_f32_u32(mask));

  tmp = vmulq_f32(fx, vdupq_n_f32(c_cephes_exp_C1));
  v4sf z = vmulq_f32(fx, vdupq_n_f32(c_cephes_exp_C2));
  x = vsubq_f32(x, tmp);
  x = vsubq_f32(x, z);

  static const float cephes_exp_p[6] = { c_cephes_exp_p0, c_cephes_exp_p1, c_cephes_exp_p2, c_cephes_exp_p3, c_cephes_exp_p4, c_cephes_exp_p5 };
  v4sf y = vld1q_dup_f32(cephes_exp_p+0);
  v4sf c1 = vld1q_dup_f32(cephes_exp_p+1);
  v4sf c2 = vld1q_dup_f32(cephes_exp_p+2);
  v4sf c3 = vld1q_dup_f32(cephes_exp_p+3);
  v4sf c4 = vld1q_dup_f32(cephes_exp_p+4);
  v4sf c5 = vld1q_dup_f32(cephes_exp_p+5);

  y = vmulq_f32(y, x);
  z = vmulq_f32(x,x);
  y = vaddq_f32(y, c1);
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, c2);
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, c3);
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, c4);
  y = vmulq_f32(y, x);
  y = vaddq_f32(y, c5);

  y = vmulq_f32(y, z);
  y = vaddq_f32(y, x);
  y = vaddq_f32(y, one);

  /* build 2^n */
  int32x4_t mm;
  mm = vcvtq_s32_f32(fx);
  mm = vaddq_s32(mm, vdupq_n_s32(0x7f));
  mm = vshlq_n_s32(mm, 23);
  v4sf pow2n = vreinterpretq_f32_s32(mm);

  y = vmulq_f32(y, pow2n);
  return y;
}

#define c_minus_cephes_DP1 -0.78515625
#define c_minus_cephes_DP2 -2.4187564849853515625e-4
#define c_minus_cephes_DP3 -3.77489497744594108e-8
#define c_sincof_p0 -1.9515295891E-4
#define c_sincof_p1  8.3321608736E-3
#define c_sincof_p2 -1.6666654611E-1
#define c_coscof_p0  2.443315711809948E-005
#define c_coscof_p1 -1.388731625493765E-003
#define c_coscof_p2  4.166664568298827E-002
#define c_cephes_FOPI 1.27323954473516 // 4 / M_PI

/* evaluation of 4 sines & cosines at once.

   The code is the exact rewriting of the cephes sinf function.
   Precision is excellent as long as x < 8192 (I did not bother to
   take into account the special handling they have for greater values
   -- it does not return garbage for arguments over 8192, though, but
   the extra precision is missing).

   Note that it is such that sinf((float)M_PI) = 8.74e-8, which is the
   surprising but correct result.

   Note also that when you compute sin(x), cos(x) is available at
   almost no extra price so both sin_ps and cos_ps make use of
   sincos_ps..
  */
static inline __attribute__((optimize("Ofast"), always_inline)) void
sincos_ps(v4sf x, v4sf *ysin, v4sf *ycos) { // any x
  v4sf xmm1, xmm2, xmm3, y;

  v4su emm2;

  v4su sign_mask_sin, sign_mask_cos;
  sign_mask_sin = vcltq_f32(x, vdupq_n_f32(0));
  x = vabsq_f32(x);

  /* scale by 4/Pi */
  y = vmulq_f32(x, vdupq_n_f32(c_cephes_FOPI));

  /* store the integer part of y in mm0 */
  emm2 = vcvtq_u32_f32(y);
  /* j=(j+1) & (~1) (see the cephes sources) */
  emm2 = vaddq_u32(emm2, vdupq_n_u32(1));
  emm2 = vandq_u32(emm2, vdupq_n_u32(~1));
  y = vcvtq_f32_u32(emm2);

  /* get the polynom selection mask
     there is one polynom for 0 <= x <= Pi/4
     and another one for Pi/4<x<=Pi/2

     Both branches will be computed.
  */
  v4su poly_mask = vtstq_u32(emm2, vdupq_n_u32(2));

  /* The magic pass: "Extended precision modular arithmetic"
     x = ((x - y * DP1) - y * DP2) - y * DP3; */
  xmm1 = vmulq_n_f32(y, c_minus_cephes_DP1);
  xmm2 = vmulq_n_f32(y, c_minus_cephes_DP2);
  xmm3 = vmulq_n_f32(y, c_minus_cephes_DP3);
  x = vaddq_f32(x, xmm1);
  x = vaddq_f32(x, xmm2);
  x = vaddq_f32(x, xmm3);

  sign_mask_sin = veorq_u32(sign_mask_sin, vtstq_u32(emm2, vdupq_n_u32(4)));
  sign_mask_cos = vtstq_u32(vsubq_u32(emm2, vdupq_n_u32(2)), vdupq_n_u32(4));

  /* Evaluate the first polynom  (0 <= x <= Pi/4) in y1,
     and the second polynom      (Pi/4 <= x <= 0) in y2 */
  v4sf z = vmulq_f32(x,x);
  v4sf y1, y2;

  y1 = vmulq_n_f32(z, c_coscof_p0);
  y2 = vmulq_n_f32(z, c_sincof_p0);
  y1 = vaddq_f32(y1, vdupq_n_f32(c_coscof_p1));
  y2 = vaddq_f32(y2, vdupq_n_f32(c_sincof_p1));
  y1 = vmulq_f32(y1, z);
  y2 = vmulq_f32(y2, z);
  y1 = vaddq_f32(y1, vdupq_n_f32(c_coscof_p2));
  y2 = vaddq_f32(y2, vdupq_n_f32(c_sincof_p2));
  y1 = vmulq_f32(y1, z);
  y2 = vmulq_f32(y2, z);
  y1 = vmulq_f32(y1, z);
  y2 = vmulq_f32(y2, x);
  y1 = vsubq_f32(y1, vmulq_f32(z, vdupq_n_f32(0.5f)));
  y2 = vaddq_f32(y2, x);
  y1 = vaddq_f32(y1, vdupq_n_f32(1));

  /* select the correct result from the two polynoms */
  v4sf ys = vbslq_f32(poly_mask, y1, y2);
  v4sf yc = vbslq_f32(poly_mask, y2, y1);
  *ysin = vbslq_f32(sign_mask_sin, vnegq_f32(ys), ys);
  *ycos = vbslq_f32(sign_mask_cos, yc, vnegq_f32(yc));
}
static inline __attribute__((optimize("Ofast"), always_inline)) v4sf
sin_ps(v4sf x) {
  v4sf ysin, ycos;
  sincos_ps(x, &ysin, &ycos);
  return ysin;
}
static inline __attribute__((optimize("Ofast"), always_inline)) v4sf
cos_ps(v4sf x) {
  v4sf ysin, ycos;
  sincos_ps(x, &ysin, &ycos);
  return ycos;
}

/**
 * Fast NEON division: num / den
 * Uses reciprocal estimate + 1 Newton-Raphson refinement step.
 */
inline float32x4_t fast_div_neon(float32x4_t num, float32x4_t den) {
  // 1. Initial estimate of (1.0 / den)
  // Accurate to about ~8 bits
  float32x4_t recip = vrecpeq_f32(den);

  // 2. Newton-Raphson step to improve accuracy to ~16 bits
  // step = 2.0 - (den * recip)
  float32x4_t step = vrecpsq_f32(den, recip);

  // recip = recip * step
  recip = vmulq_f32(recip, step);

  // Optional: Add a second step if you need full 24-bit+ f32 precision,
  // though 1 step is often fine for filter coefficients/audio.
  // step = vrecpsq_f32(den, recip);
  // recip = vmulq_f32(recip, step);

  // 3. Multiply numerator by the reciprocal (num * 1/den)
  return vmulq_f32(num, recip);
}

/**
 * ARMv7-compatible vectorized exp(x) for 4 floats using e_expff approximation.
 * Replacement for vexpq_f32 which is ARMv8/AArch64 only.
 */
static inline float32x4_t neon_expq_f32(float32x4_t x) {
  float32_t buf[4];
  vst1q_f32(buf, x);
  buf[0] = e_expff(buf[0]);
  buf[1] = e_expff(buf[1]);
  buf[2] = e_expff(buf[2]);
  buf[3] = e_expff(buf[3]);
  return vld1q_f32(buf);
}

/**
 * ARMv7-compatible vectorized sqrt(x) for 4 floats using NEON rsqrte + N-R.
 * Replacement for vsqrtq_f32 which is ARMv8/AArch64 only.
 */
static inline float32x4_t neon_sqrtq_f32(float32x4_t x) {
  /* Guard against x=0 to avoid 1/sqrt(0)=inf */
  float32x4_t safe_x = vmaxq_f32(x, vdupq_n_f32(1e-30f));
  float32x4_t inv_sqrt = vrsqrteq_f32(safe_x);
  /* One Newton-Raphson step: inv_sqrt *= (3 - x * inv_sqrt^2) / 2 */
  inv_sqrt = vmulq_f32(vrsqrtsq_f32(vmulq_f32(safe_x, inv_sqrt), inv_sqrt), inv_sqrt);
  return vmulq_f32(safe_x, inv_sqrt);
}

/** @} */

#endif // __float_math_h

/** @} @} */


#endif /* A4247454_9632_47F2_AD08_C3E0FB9034E2 */


#endif /* BBAC8CC0_8473_452E_801A_60AD28A72398 */
