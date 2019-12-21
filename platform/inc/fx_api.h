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
 * @file    fx_api.h
 * @brief   Effects runtime API.
 *
 * @addtogroup fx Effects
 * @{
 * 
 * @addtogroup fx_api Runtime API
 * @{
 */

#ifndef __fx_api_h
#define __fx_api_h

#include "float_math.h"
#include "int_math.h"
#include "fixed_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define __fast_inline static inline __attribute__((always_inline, optimize("Ofast")))  
  
  /*===========================================================================*/
  /* Runtime Environment                                                       */
  /*===========================================================================*/

  /**
   * @name   Runtime Environment
   * @{ 
   */
  
  /**
   * Current platform
   */
  extern const uint32_t k_fx_api_platform;
  
  /**
   * Current API version
   */
  extern const uint32_t k_fx_api_version;
  
  /**
   * Get MCU hash
   *
   * @return  A MCU specific "unique" hash.
   */
  uint32_t _fx_mcu_hash(void);

  __fast_inline uint32_t fx_mcu_hash(void) {
    return _fx_mcu_hash();
  }

  /**
   * Get current tempo as beats per minute as integer
   *
   * @return  Current integer BPM, multiplied by 10 to allow 1 decimal precision 
   */
  uint16_t _fx_get_bpm(void);

  __fast_inline uint16_t fx_get_bpm(void) {
    return _fx_get_bpm();
  }

  /**
   * Get current tempo as beats per minute as floating point
   *
   * @return  Current floating point BPM
   */
  float _fx_get_bpmf(void);

  __fast_inline float fx_get_bpmf(void) {
    return _fx_get_bpmf();
  }

  /** @} */
  
  /*===========================================================================*/
  /* Lookup tables                                                             */
  /*===========================================================================*/
  
  /**
   * @name   Sine half-wave
   * @note   Wrap and negate for phase >= 0.5
   * @{ 
   */
  
#define k_wt_sine_size_exp     (7)
#define k_wt_sine_size         (1U<<k_wt_sine_size_exp)
#define k_wt_sine_u32shift     (24)
#define k_wt_sine_frrecip      (5.96046447753906e-008f) // 1/(1<<24)
#define k_wt_sine_mask         (k_wt_sine_size-1)
#define k_wt_sine_lut_size     (k_wt_sine_size+1)
  
  extern const float wt_sine_lut_f[k_wt_sine_lut_size];

  /**
   * Lookup value of sin(2*pi*x).
   *
   * @param   x  Phase ratio
   * @return     Result of sin(2*pi*x).
   */
  __fast_inline float fx_sinf(float x) {
    const float p = x - (uint32_t)x;
    
    // half period stored -- wrap around and invert
    const float x0f = 2.f * p * k_wt_sine_size;
    const uint32_t x0p = (uint32_t)x0f;

    const uint32_t x0 = x0p & k_wt_sine_mask;
    const uint32_t x1 = (x0 + 1) & k_wt_sine_mask;
    
    const float y0 = linintf(x0f - x0p, wt_sine_lut_f[x0], wt_sine_lut_f[x1]);
    return (x0p < k_wt_sine_size)?y0:-y0;
  }
  
  /**
   * Lookup value of sin(2*pi*x) in [0, 1.0] range.
   *
   * @param   x  Phase ratio.
   * @return     Result of sin(2*pi*x).
   */
  __fast_inline float fx_sinuf(uint32_t x) {
    (void)x;
    return 0.f;
  }
  
  /**
   * Lookup value of cos(2*pi*x) in [0, 1.0] range.
   *
   * @param   x  Value in [0, 1.0].
   * @return     Result of cos(2*pi*x).
   */
  __fast_inline float fx_cosf(float x) {
    return fx_sinf(x+0.25f);
  }

  /**
   * Lookup value of cos(2*pi*x) in [0, 1.0] range.
   *
   * @param   x  Phase ratio.
   * @return     Result of sin(2*pi*x).
   */
  __fast_inline float fx_cosuf(uint32_t x) {
    return fx_sinuf(x+((k_wt_sine_size>>2)<<k_wt_sine_u32shift));
  }
  
  /** @} */
    
  /*===========================================================================*/
  /* Various function lookups                                                  */
  /*===========================================================================*/
  
  /**
   * @name   Various function lookups
   *
   * @{ 
   */
  
#define k_log_size_exp         (8)
#define k_log_size             (1U<<k_log_size_exp)
#define k_log_mask             (k_log_size-1)
#define k_log_lut_size         (k_log_size+1)
  
  extern const float log_lut_f[k_log_lut_size];

  /**
   * Lookup value of log(x) in [0.00001, 1.0] range.
   *
   * @param   x  Value in [0.00001, 1.0].
   * @return     Result of log(x).
   * @note Not checking input, caller responsible for bounding x.
   */
  __fast_inline float fx_logf(float x) {
    const float idxf = x * k_log_size;
    const uint32_t idx = (uint32_t)idxf;
    const float y0 = log_lut_f[idx];
    const float y1 = log_lut_f[idx+1];
    return linintf(idxf - idx, y0, y1);
  }

#define k_tanpi_size_exp         (8)
#define k_tanpi_size             (1U<<k_tanpi_size_exp)
#define k_tanpi_mask             (k_tanpi_size-1)
#define k_tanpi_range_recip      (2.04081632653061f) // 1/0.49
#define k_tanpi_lut_size         (k_tanpi_size+1)
  
  extern const float tanpi_lut_f[k_log_lut_size];
  
  /**
   * Lookup value of tan(pi*x) in [0.0001, 0.49] range.
   *
   * @param   x  Value in [0.0001, 0.49].
   * @return     Result of tan(pi*x).
   * @note Not checking input, caller responsible for bounding x.
   */
  __fast_inline float fx_tanpif(float x) {
    const float idxf = x * k_tanpi_range_recip * k_tanpi_size;
    const uint32_t idx = (uint32_t)idxf;
    const float y0 = tanpi_lut_f[idx];
    const float y1 = tanpi_lut_f[idx+1];
    return linintf(idxf - idx, y0, y1);
  }

#define k_sqrtm2log_size_exp         (8)
#define k_sqrtm2log_size             (1U<<k_sqrtm2log_size_exp)
#define k_sqrtm2log_mask             (k_sqrtm2log_size-1)
#define k_sqrtm2log_base             (0.005f)
#define k_sqrtm2log_range_recip      (1.00502512562814f) // 1/0.995
#define k_sqrtm2log_lut_size         (k_sqrtm2log_size+1)
  
  extern const float sqrtm2log_lut_f[k_sqrtm2log_lut_size];
  
  /**
   * Lookup value of sqrt(-2*log(x)) in [0.005, 1.0] range.
   *
   * @param   x  Value in [0.005, 1.0].
   * @return     Result of sqrt(-2*log(x)).
   * @note Not checking input, caller responsible for bounding x.
   */
  __fast_inline float fx_sqrtm2logf(float x) {
    const float idxf = (x-k_sqrtm2log_base) * k_sqrtm2log_range_recip * k_sqrtm2log_size;
    const uint32_t idx = (uint32_t)idxf;
    const float y0 = sqrtm2log_lut_f[idx];
    const float y1 = sqrtm2log_lut_f[idx+1];
    return linintf(idxf - idx, y0, y1);
  }

#define k_pow2_size_exp         (8)
#define k_pow2_size             (1U<<k_pow2_size_exp)
#define k_pow2_scale            (85.3333333333333f) // 256 / 3
#define k_pow2_mask             (k_pow2_size-1)
#define k_pow2_lut_size         (k_pow2_size+1)
  
  extern const float pow2_lut_f[k_pow2_lut_size];

  /**
   * Lookup value of 2^k for k in [0, 3.0] range.
   *
   * @param   x  Value in [0, 3.0].
   * @return     Result of 2^k.
   * @note Not checking input, caller responsible for bounding x.
   */
  __fast_inline float fx_pow2f(float x) {
    const float idxf = x * k_pow2_scale;
    const uint32_t idx = (uint32_t)idxf;
    const float y0 = pow2_lut_f[idx];
    const float y1 = pow2_lut_f[idx+1];
    return linintf(idxf - idx, y0, y1);
  }
  
  /** @} */

  /*===========================================================================*/
  /* Clipping and Saturation                                                   */
  /*===========================================================================*/
  /**
   * @name   Clipping and Saturation.
   *
   * @{ 
   */
  
  
  /**
   * Soft clip
   *
   * @param   c  Coefficient in [0, 1/3].
   * @param   x  Value in (-inf, +inf).
   * @return     Clipped value in [-(1-c), (1-c)].
   */
  __fast_inline float fx_softclipf(const float c, float x)
  {
    x = clip1m1f(x);
    return x - c * (x*x*x);
  }

#define k_cubicsat_size_exp  (7)
#define k_cubicsat_size      (1U<<k_cubicsat_size_exp)
#define k_cubicsat_mask      (k_cubicsat_size-1)
#define k_cubicsat_lut_size  (k_cubicsat_size+1)
  
  extern const float cubicsat_lut_f[k_cubicsat_lut_size];
  
  /**
   * Cubic saturation.
   *
   * @param   x  Value in [-1.0, 1.0].
   * @return     Cubic curve above 0.42264973081, gain: 1.2383127573
   */
  __fast_inline float fx_sat_cubicf(float x) {
    const float xf = si_fabsf(clip1f(x)) * k_cubicsat_size;
    const uint32_t xi = (uint32_t)x;
    const float y0 = cubicsat_lut_f[xi];
    const float y1 = cubicsat_lut_f[xi+1];
    return si_copysignf(linintf(xf - xi, y0, y1), x);
  }

#define k_schetzen_size_exp  (7)
#define k_schetzen_size      (1U<<k_schetzen_size_exp)
#define k_schetzen_mask      (k_schetzen_size-1)
#define k_schetzen_lut_size  (k_schetzen_size+1)
  
  extern const float schetzen_lut_f[k_schetzen_lut_size];
  
  /**
   * Schetzen saturation.
   *
   * @param   x  Value in [-1.0, 1.0].
   * @return     Saturated value.
   */
  __fast_inline float fx_sat_schetzenf(float x) {
    const float xf = si_fabsf(clip1f(x)) * k_schetzen_size;
    const uint32_t xi = (uint32_t)x;
    const float y0 = schetzen_lut_f[xi];
    const float y1 = schetzen_lut_f[xi+1];
    return si_copysignf(linintf(xf - xi, y0, y1), x);
  }

  /** @} */

  /**
   * @name   Bit reduction.
   *
   * @{ 
   */
  
#define k_bitres_size_exp    (7)
#define k_bitres_size        (1U<<k_bitres_size_exp)
#define k_bitres_mask        (k_bitres_size-1)
#define k_bitres_lut_size    (k_bitres_size+1)
  
  extern const float bitres_lut_f[k_bitres_lut_size];
  
  /**
   * Bit depth scaling table
   *
   * @param   x  Value in [0, 1.0].
   * @return     Quantization scaling factor.
   * @note       Fractional bit depth, exponentially mapped, 1 to 24 bits.
   */
  __fast_inline float fx_bitresf(float x) {
    const float xf = x * k_bitres_size;
    const uint32_t xi = (uint32_t)xf;
    const float y0 = bitres_lut_f[xi];
    const float y1 = bitres_lut_f[xi+1];
    return linintf(xf - xi, y0, y1);
  }

  /** @} */
  
  /*===========================================================================*/
  /* Noise source                                                              */
  /*===========================================================================*/

  /**
   * @name   Noise source
   * @{ 
   */
  
  /**
   * Random integer
   *
   * @return     Value in [0, UINT_MAX].
   * @note       Generated with Park-Miller-Carta
   */
  uint32_t _fx_rand(void);

  __fast_inline uint32_t fx_rand(void) {
    return _fx_rand();
  }
  
  /**
   * Gaussian white noise
   *
   * @return     Value in [-1.0, 1.0].
   */
  float _fx_white(void);

  __fast_inline float fx_white(void) {
    return _fx_white();
  }
  
  /** @} */
  
#ifdef __cplusplus
}
#endif 


#endif // __fx_api_h

/** @} @} */
