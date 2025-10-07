/*
 * File: bitres_lut.h
 *
 * powf(2.f, 1.f + 23.f * (1.f - powf(bitcrush_amt, 0.125f))) - 1;
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017-2022 (c) Korg
 *
 */

/**
 * @file    bitres_lut.h
 * @brief   powf(2.f, 1.f + 23.f * (1.f - powf(bitcrush_amt, 0.125f))) - 1;
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __bitres_lut_h
#define __bitres_lut_h

// #include "utils/fixed_math.h"
#include "utils/float_math.h"

#ifdef __cplusplus
extern "C" {
#endif

  
#define k_bitres_size             128
#define k_bitres_mask             (k_bitres_size-1)
#define k_bitres_lut_size         (k_bitres_size+1)
  
  extern const float bitres_lut_f[k_bitres_lut_size];
  
  static inline __attribute__((optimize("Ofast"),always_inline))
  float si_bitresf(const float x) {
    const float idxf = x * k_bitres_size;
    const uint32_t idx = (uint32_t)idxf;
    const float fr = idxf - idx;
    const float y0 = bitres_lut_f[idx];
    const float y1 = bitres_lut_f[idx+1];
    return y0 + fr * (y1 - y0);
  }

  
#ifdef __cplusplus
}
#endif

  
#endif // __log_lut_h

/** @} */

