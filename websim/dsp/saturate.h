/*
 *  File: saturate.h
 *
 *  Saturation utils
 *
 *  Author: Etienne Noreau-Hebert
 *
 *  2017-2022 (c) Korg
 *
 */
/**
 * @file    saturate.h
 * @brief   Saturation utils
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __saturate_h
#define __saturate_h

#include "utils/float_math.h"

#include "tanh_lut.h"

#define k_cubicsat_size     128
#define k_cubicsat_mask     (k_cubicsat_size-1)
#define k_cubicsat_lut_size (k_cubicsat_size+1)
extern const float cubicsat_lut_f[k_cubicsat_lut_size];

static inline __attribute__((optimize("Ofast"),always_inline))
float saturate_cube(const float in)
{
  static const float thr = 0.42264973081f;
  static const float gain = 1.2383127573f;
  
  const float in_abs = si_fabsf(in);

  if (in_abs < thr) return gain*in;
  
  float a = si_fabsf(in) - thr;
  
  a = si_fabsf(a * a * a);
  if (in < 0)
    a = -a;
  return (in - a) * gain;
}

static inline __attribute__((optimize("Ofast"),always_inline))
float saturate_cube_lu(float x)
{
  const float xf = clip1f(si_fabsf(x)) * k_cubicsat_size;
  const uint32_t xi = (uint32_t)x;
  const float fr = xf - xi;
  const float y0 = cubicsat_lut_f[xi];
  const float y1 = cubicsat_lut_f[xi+1];
  return si_copysignf(linintf(fr, y0, y1), x);
}

/**
 * Soft clip
 *
 * @param   c  Coefficient in [0, 1/3].
 * @param   x  Value in (-inf, +inf).
 * @return     Clipped value in [-(1-c), (1-c)].
 */
static inline __attribute__((optimize("Ofast"),always_inline))
float softclip(const float c, float x)
{
  // factor = 1/3 and lower
  x = clip1m1f(x);
  return x - c * (x*x*x);
}

#define k_softclip3_comp (1.499999999999925f)

static inline __attribute__((optimize("Ofast"),always_inline))
float softclip3(float in)
{
  in = clip1m1f(in);
  return in - 0.3333333333333f * (in*in*in);
}

#define k_softclip5_comp (1.25f)

static inline __attribute__((optimize("Ofast"),always_inline))
float softclip5(float in)
{
  in = clip1m1f(in);
  return in - 0.2f * (in*in*in);
}

#define k_softclip8_comp (1.1428571428571428f)

static inline __attribute__((optimize("Ofast"),always_inline))
float softclip8(float in)
{
  in = clip1m1f(in);
  return in - 0.125f * (in*in*in);
}

#define k_softclip10_comp (1.1111111111111112f)

static inline __attribute__((optimize("Ofast"),always_inline))
float softclip10(float in)
{
  in = clip1m1f(in);
  return in - 0.1f * (in*in*in);
}

#define k_schetzen_size     128
#define k_schetzen_mask     (k_schetzen_size-1)
#define k_schetzen_lut_size (k_schetzen_size+1)
extern const float schetzen_lut_f[k_schetzen_lut_size];

static inline __attribute__((optimize("Ofast"),always_inline))
float schetzenf(const float x0) {
  static const float thr = 1.f/3.f;
  static const float thr2 = 2.f/3.f;
  const float x0a = si_fabsf(x0);
  const float c = 2.f - 3.f * x0a;
  //TODO: implement with fsel
  return (x0a < thr) ? 2.f * x0 : (x0a > thr2) ? si_copysignf(1.f, x0) : si_copysignf((3.f - c*c),x0) * thr;
}

static inline __attribute__((optimize("Ofast"),always_inline))
float schetzen_lu(float x)
{
  const float xf = clip1m1f(si_fabsf(x)) * k_schetzen_size;
  const uint32_t xi = (uint32_t)x;
  const float fr = xf - xi;
  const float y0 = schetzen_lut_f[xi];
  const float y1 = schetzen_lut_f[xi+1];
  return si_copysignf(linintf(fr, y0, y1), x);
}

static inline __attribute__((optimize("Ofast"),always_inline))
float saturate_tanh(const float in) {
  return si_tanhf(in);
}

#endif // __saturate_h

/** @} */
