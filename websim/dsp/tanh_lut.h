/*
 * File: tanh_lut.h
 *
 * tanh(x)
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017-2022 (c) Korg
 *
 */

/**
 * @file    tanh_lut.h
 * @brief   tanh(x)
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __tanh_lut_h
#define __tanh_lut_h

// #include "utils/fixed_math.h"
#include "utils/float_math.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * @name   LUT for tanh(x) in [-10, 10]
   * @{ 
   */
  
#define k_tanh_size             1024
#define k_tanh_half_size        (k_tanh_size>>1)
#define k_tanh_mask             (k_tanh_size-1)
#define k_tanh_lut_size         (k_tanh_size+1)
#define k_tanh_range_f          10.f
#define k_tanh_zero_offset      512
extern const float tanh_lut_f[k_tanh_lut_size];
  
/** @} */

  /**
   * @brief tanh(x) lookup function
   * @note Not checking input, caller responsible for bounding x in [-10, 10]
   */
  static inline __attribute__((optimize("Ofast"),always_inline))
  float si_tanhf(const float x) {
    static const float recip = 1.f / k_tanh_range_f;
    const float idxf = k_tanh_zero_offset + x * recip * k_tanh_half_size;
    const uint32_t idx = (uint32_t)idxf;
    const float fr = idxf - idx;
    const float y0 = tanh_lut_f[idx];
    const float y1 = tanh_lut_f[idx+1];
    return y0 + fr * (y1 - y0);
  }

  
#ifdef __cplusplus
}
#endif

  
#endif // __wavetable_lut_h

/** @} */

