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
 * @addtogroup UTILS
 * @{
 */


#ifndef __float_math_h
#define __float_math_h

#include <math.h>
#include <stdint.h>

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
#define M_LOG2E 1.44269504088896f
#endif

#ifndef M_LOG10E
#define M_LOG10E 0.4342944819032518f
#endif

#ifndef M_LN2
#define M_LN2 0.6931471805599453094f
#endif

#ifndef M_LN10
#define M_LN10 2.30258509299404568402f
#endif

#ifndef M_PI
#define M_PI 3.141592653589793f
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

static inline __attribute__((optimize("Ofast"),always_inline))
float fsel(const float a, const float b, const float c) {
  return (a >= 0) ? b : c;
}

static inline __attribute__((optimize("Ofast"),always_inline))
uint8_t fselb(const float a) {
  return (a >= 0) ? 1 : 0;
}


static inline __attribute__((always_inline))
uint8_t float_is_neg(const f32_t f) {
  return (f.i >> 31) != 0;
}

static inline __attribute__((always_inline))
int32_t float_mantissa(f32_t f) {
  return f.i & ((1 << 23) - 1);
}

static inline __attribute__((always_inline))
int32_t float_exponent(f32_t f) {
  return (f.i >> 23) & 0xFF;
}


static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_add(const f32pair_t p0, const f32pair_t p1) {
  return (f32pair_t){p0.a + p1.a, p0.b + p1.b};
}

static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_sub(const f32pair_t p0, const f32pair_t p1) {
  return (f32pair_t){p0.a - p1.a, p0.b - p1.b};
}

static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_addscal(const f32pair_t p, const float scl) {
  return (f32pair_t){p.a + scl, p.b + scl};
}

static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_mul(const f32pair_t p0, const f32pair_t p1) {
  return (f32pair_t){p0.a * p1.a, p0.b * p1.b};
}

static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_mulscal(const f32pair_t p, const float scl) {
  return (f32pair_t){p.a * scl, p.b * scl};
}

static inline __attribute__((optimize("Ofast"),always_inline))
f32pair_t f32pair_linint(const float fr, const f32pair_t p0, const f32pair_t p1) {
  const float frinv = 1.f - fr;
  return (f32pair_t){ frinv * p0.a + fr * p1.a, frinv * p0.b + fr * p1.b };
}

static inline __attribute__((optimize("Ofast"),always_inline))
float si_copysignf(const float x, const float y)
{
  f32_t xs = {x};
  f32_t ys = {y};
  
  xs.i &= 0x7fffffff;
  xs.i |= ys.i & 0x80000000;
  
  return xs.f;
}

static inline __attribute__((optimize("Ofast"),always_inline))
float si_fabsf(float x)
{
  f32_t xs = {x};
  xs.i &= 0x7fffffff;
  return xs.f;
}

static inline __attribute__((optimize("Ofast"),always_inline))
float si_floorf(float x)
{
  return (float)((uint32_t)x);
}

static inline __attribute__((optimize("Ofast"),always_inline))
float si_ceilf(float x)
{
  return (float)((uint32_t)x + 1);
}

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

static inline __attribute__((optimize("Ofast"), always_inline))
float clipmaxf(const float x, const float m)
{ return (((x)>=m)?m:(x)); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clipminf(const float  m, const float x)
{ return (((x)<=m)?m:(x)); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clipminmaxf(const float min, const float x, const float max)
{ return (((x)>=max)?max:((x)<=min)?min:(x)); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clip0f(const float x)
{ return (((x)<0.f)?0.f:(x)); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clip1f(const float x)
{ return (((x)>1.f)?1.f:(x)); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clip01f(const float x)
{ return (((x)>1.f)?1.f:((x)<0.f)?0.f:(x)); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clipm1f(const float x)
{ return (((x)<-1.f)?-1.f:(x)); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clip1m1f(const float x)
{ return (((x)>1.f)?1.f:((x)<-1.f)?-1.f:(x)); }

#else

static inline __attribute__((optimize("Ofast"), always_inline))
float clipmaxf(const float x, const float m)
{ return clampmaxfsel(x, m); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clipminf(const float  m, const float x)
{ return clampminfsel(m, x); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clipminmaxf(const float min, const float x, const float max)
{ return clampfsel(min, x, max); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clip0f(const float x)
{ return clampminfsel(0, x); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clip1f(const float x)
{ return clampmaxfsel(x, 1); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clip01f(const float x)
{ return clampfsel(0, x, 1); }

static inline __attribute__((optimize("Ofast"), always_inline))
float clipm1f(const float x)
{ return clampminfsel(-1, x); }

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
 * @note    Use with care. In some cases can only be slightly faster than libc calls.
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

static inline __attribute__((optimize("Ofast"), always_inline))
float fastercosf(float x) {
  static const float p = 0.54641335845679634f;
  union { float f; uint32_t i; } vx = { x };
  vx.i &= 0x7FFFFFFF;
  const float qpprox = 1.0f - M_2_PI * vx.f;
  return qpprox + p * qpprox * (1.0f - qpprox * qpprox);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float fastersinfullf(float x) {
  const int32_t k = (int32_t)(x * M_1_TWOPI);
  const float half = (x < 0) ? -0.5f : 0.5f;
  return fastersinf((half + k) * M_TWOPI - x);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float fastercosfullf(float x) {
  return fastersinfullf(x + M_PI_2);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float fastertanfullf(float x) {
  const int32_t k = (int32_t)(x * M_1_TWOPI);
  const float half = (x < 0) ? -0.5f : 0.5f;
  const float xnew = x - (half + k) * M_TWOPI;
  return fastersinf(xnew)/fastercosf(xnew);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float fasterpow2f(float p) {
  float clipp = (p < -126) ? -126.0f : p;
  union { uint32_t i; float f; } v = { (uint32_t)( (1 << 23) * (clipp + 126.94269504f) ) };
  return v.f;
}

static inline __attribute__((optimize("Ofast"), always_inline))
float fasterexpf(float p) {
  return fasterpow2f(1.442695040f * p);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float fasterlog2f(float x) {
  union { float f; uint32_t i; } vx = { x };
  float y = (float)(vx.i);
  y *= 1.1920928955078125e-7f;
  return y - 126.94269504f;
}

static inline __attribute__((optimize("Ofast"), always_inline))
float fasterpowf(float x, float p) {
  return fasterpow2f(p * fasterlog2f(x));
}

// from http://dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization
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

// http://math.stackexchange.com/questions/107292/rapid-approximation-of-tanhx
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
 * @note    These are very slow, use with caution. Should use table lookups in performance critical sections.
 * @{
 */

static inline __attribute__((optimize("Ofast"), always_inline))
float ampdbf(const float amp) {
  return (amp < 0.f) ? -999.f : 20.f*log10f(amp);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float dbampf(const float db) {
  return powf(10.f,0.05f*db);
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

static inline __attribute__((optimize("Ofast"), always_inline))
float linintf(const float fr, const float x0, const float x1) {
  return x0 + fr * (x1 - x0);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float cosintf(const float fr, const float x0, const float x1) {
  const float tmp = (1.f - fastercosfullf(fr * M_PI)) * 0.5f;
  return x0 + tmp * (x1 - x0);
}

/** @} */

#endif // __float_math_h

/** @} */

