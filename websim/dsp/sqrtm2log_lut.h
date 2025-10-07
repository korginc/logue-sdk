/*
 * File: sqrtm2log_lut.h
 *
 * sqrt(-2*log(x)) - 0.005 to 1.0
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017-2022 (c) Korg
 *
 */

/**
 * @file    sqrtm2log_lut.h
 * @brief   sqrt(-2 * log(x)) - 0.005 to 1.0
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __sqrtm2log_lut_h
#define __sqrtm2log_lut_h

// #include "utils/fixed_math.h"
#include "utils/float_math.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * @name   LUT for sqrt(-2*log(x)) in [0.005, 1.0]
   * @{ 
   */
  
#define k_sqrtm2log_size             256
#define k_sqrtm2log_mask             (k_sqrtm2log_size-1)
#define k_sqrtm2log_lut_size         (k_sqrtm2log_size+1)
#define k_sqrtm2log_base_f           0.005
#define k_sqrtm2log_range_f          0.995
extern const float sqrtm2log_lut_f[k_sqrtm2log_lut_size];
  
/** @} */

  /**
   * @brief sqrtm2log(x) lookup function
   * @note Not checking input, caller responsible for bounding x in [0.005, 1.0]
   */
  static inline __attribute__((optimize("Ofast"),always_inline))
  float si_sqrtm2logf(const float x) {
    static const float recip = 1.f / k_sqrtm2log_range_f;
    const float idxf = clipminf(0, x-k_sqrtm2log_base_f) * recip * k_sqrtm2log_size;
    const uint32_t idx = (uint32_t)idxf;
    const float fr = idxf - idx;
    const float y0 = sqrtm2log_lut_f[idx];
    const float y1 = sqrtm2log_lut_f[idx+1];
    return y0 + fr * (y1 - y0);
  }

  
#ifdef __cplusplus
}
#endif

  
#endif // __sqrtm2log_lut_h

/** @} */

