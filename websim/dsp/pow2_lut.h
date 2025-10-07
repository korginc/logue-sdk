/*
 * File: pow2_lut.h
 *
 * Lookup Tables for powers of two.
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017-2022 (c) Korg
 *
 */

/**
 * @file    pow2_lut.h
 * @brief   Lookup Tables for powers of two.
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __pow2_lut_h
#define __pow2_lut_h

#include "utils/float_math.h"

// 2^k for k=0-3
#define k_pow2_lut_scale        (256)
#define k_pow2_lut_size         (257)
#define k_pow2_lut_scale_factor (85.3333333333333f) // 256 / 3
extern const float pow2_lut_f[k_pow2_lut_size];

static inline __attribute__((optimize("Ofast"), always_inline))
float si_pow2f(const float x) {
  const float idxf = x * k_pow2_lut_scale_factor;
  const uint32_t base = (uint32_t)idxf;
  const float y0 = pow2_lut_f[base];
  const float y1 = pow2_lut_f[base+1];
  return linintf(idxf - base, y0, y1);
}

#endif // __pow2_lut_h

/** @} */

