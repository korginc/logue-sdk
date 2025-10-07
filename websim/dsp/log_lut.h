/*
 * File: log_lut.h
 *
 * log(x) - 0.00001 to 1.0
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017-2022 (c) Korg
 *
 */

/**
 * @file    log_lut.h
 * @brief   log(x) - 0.00001 to 1.0
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __log_lut_h
#define __log_lut_h

// #include "utils/fixed_math.h"
#include "utils/float_math.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * @name   LUT for log(x) in [0.00001, 1.0]
   * @{ 
   */
  
#define k_log_size             256
#define k_log_mask             (k_log_size-1)
#define k_log_lut_size         (k_log_size+1)
#define k_log_range_f          1.0
extern const float log_lut_f[k_log_lut_size];
  
/** @} */

  /**
   * @brief log(x) lookup function
   * @note Not checking input, caller responsible for bounding x in [0.00001, 1.0]
   */
  static inline __attribute__((optimize("Ofast"),always_inline))
  float si_logf(const float x) {
    static const float recip = 1.f / k_log_range_f;
    const float idxf = x * recip * k_log_size;
    const uint32_t idx = (uint32_t)idxf;
    const float fr = idxf - idx;
    const float y0 = log_lut_f[idx];
    const float y1 = log_lut_f[idx+1];
    return y0 + fr * (y1 - y0);
  }

  
#ifdef __cplusplus
}
#endif

  
#endif // __log_lut_h

/** @} */

