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

#include <math.h>
#include <stdint.h>
#include <arm_neon.h>

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
  return qpprox * (q + p.f * qpprox);
}

/** "Fast" sine approximation, valid on full x domain
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastsinfullf(float x) {
  const int32_t k = (int32_t)(x * M_1_TWOPI);
  const float half = (x < 0) ? -0.5f : 0.5f;
  return fastsinf((half + k) * M_TWOPI - x);
}

/** "Faster" sine approximation, valid on full x domain
 * @note Adapted from Paul Mineiro's FastFloat
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastersinfullf(float x) {
  const int32_t k = (int32_t)(x * M_1_TWOPI);
  const float half = (x < 0) ? -0.5f : 0.5f;
  return fastersinf((half + k) * M_TWOPI - x);
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

/** "Fast" cosine approximation, valid on full x domain
 * @note Adapted from Paul Mineiro's FastFloat
 * @note Warning: can be slower than libc version!
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float fastcosfullf(float x) {
  return fastersinfullf(x + M_PI_2);
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

/** "Fast" natural logarithm approximation, valid for positive x as precision allows.
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

/** "Fast" power of 2 approximation, valid for x in [ -126, ... as precision allows.
 * @note Adapted from Paul Mineiro's FastFloat
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


/** Fast but imprecise approximation for the euclidean distance
 * given carthesian coordinates for two points (x1,y1) and (x2,y2)
 * https://en.m.wikibooks.org/wiki/Algorithms/Distance_approximations
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float eucDist1(const float x1, const float y1, const float x2, const float y2) {
    auto dx = fabs(x2 - x1);
    auto dy = fabs(y2 - y1);
    return sqrtsum2(dx, dy);
}


/** A fast approximation of 2D distance based on an octagonal boundary
 * given carthesian coordinates for two points (x1,y1) and (x2,y2)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float eucDist2(const float x1, const float y1, const float x2, const float y2) {
    auto dx = fabs(x2 - x1);
    auto dy = fabs(y2 - y1);
    return sqrtsum2acc(dx, dy);
}


/** A better approximation for the euclidean distance
 * given carthesian coordinates for two points (x1,y1) and (x2,y2)
 * https://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
 */
static inline __attribute__((optimize("Ofast"), always_inline))
float eucDist2Bett(const float x1, const float y1, const float x2, const float y2) {
    auto dx = fabs(x2 - x1);
    auto dy = fabs(y2 - y1);
    return sqrtsum2bett(dx, dy);
}


/** @} */

#endif // __float_math_h

/** @} @} */

