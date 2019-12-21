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
 * @file    osc_api.h
 * @brief   Oscillator runtime API.
 *
 * @addtogroup osc Oscillator
 * @{
 *
 * @addtogroup osc_api Runtime API
 * @{
 */

#ifndef __osc_api_h
#define __osc_api_h

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
  extern const uint32_t k_osc_api_platform;
  
  /**
   * Current API version
   */
  extern const uint32_t k_osc_api_version;
  
  /**
   * Get MCU hash
   *
   * @return  A MCU specific "unique" hash.
   */
  uint32_t _osc_mcu_hash(void);

  __fast_inline uint32_t osc_mcu_hash(void) {
    return _osc_mcu_hash();
  }

#define k_samplerate        (48000)
#define k_samplerate_recipf (2.08333333333333e-005f)

  /** @} */
  
  /*===========================================================================*/
  /* Lookup tables                                                             */
  /*===========================================================================*/

  /**
   * @name   Note to frequency conversion table
   * @{ 
   */
  
#define k_midi_to_hz_size      (152)
  
  extern const float midi_to_hz_lut_f[k_midi_to_hz_size];

#define k_note_mod_fscale      (0.00392156862745098f)
#define k_note_max_hz          (23679.643054f)
  
  /**
   * Get Hertz value for note
   *
   * @param note Note in [0-151] range.
   * @return     Corresponding Hertz value.
   */
  __fast_inline float osc_notehzf(uint8_t note) {
    return midi_to_hz_lut_f[clipmaxu32(note,k_midi_to_hz_size-1)];
  }

  /**
   * Get floating point phase increment for given note and fine modulation
   *
   * @param note Note in [0-151] range, mod in [0-255] range.
   * @return     Corresponding 0-1 phase increment in floating point.
   */
  __fast_inline float osc_w0f_for_note(uint8_t note, uint8_t mod) {    
    const float f0 = osc_notehzf(note);
    const float f1 = osc_notehzf(note+1);
    
    const float f = clipmaxf(linintf(mod * k_note_mod_fscale, f0, f1), k_note_max_hz);

    return f * k_samplerate_recipf;
  }
  
  /** @} */
  
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
  __fast_inline float osc_sinf(float x) {
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
   * Lookup value of cos(2*pi*x) in [0, 1.0] range.
   *
   * @param   x  Value in [0, 1.0].
   * @return     Result of cos(2*pi*x).
   */
  __fast_inline float osc_cosf(float x) {
    return osc_sinf(x+0.25f);
  }
  
  /** @} */
  
/**
 * @name   Band-limited sawtooth half-waves
 * @note   Wrap interpolation towards zero for 0.5, negate and reverse phase for >= 0.5
 * @{ 
 */

#define k_wt_saw_size_exp      (7)
#define k_wt_saw_size          (1U<<k_wt_saw_size_exp)
#define k_wt_saw_u32shift      (24)
#define k_wt_saw_frrecip       (5.96046447753906e-008f) // 1/(1<<24)  
#define k_wt_saw_mask          (k_wt_saw_size-1)
#define k_wt_saw_lut_size      (k_wt_saw_size+1)
#define k_wt_saw_notes_cnt     (7)
#define k_wt_saw_lut_tsize     (k_wt_saw_notes_cnt * k_wt_saw_lut_size)
  
  extern const uint8_t wt_saw_notes[k_wt_saw_notes_cnt];
  extern const float wt_saw_lut_f[k_wt_saw_lut_tsize];

  //TODO: add integer phase versions
  
  /**
   * Sawtooth wave lookup.
   *
   * @param   x  Phase in [0, 1.0].
   * @return     Wave sample.
   */
  __fast_inline float osc_sawf(float x) {
    const float p = x - (uint32_t)x;
    
    const float x0f = 2.f * p * k_wt_saw_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    uint32_t x0 = x0p, x1 = x0p+1;
    float sign = 1.f;
    if (x0p >= k_wt_saw_size) {
      x0 = k_wt_saw_size - (x0p & k_wt_saw_mask);
      x1 = x0 - 1;
      sign = -1.f;
    }
    
    const float y0 = linintf(x0f - x0p, wt_saw_lut_f[x0], wt_saw_lut_f[x1]);
    return sign*y0;
  }
  
  /**
   * Band-limited sawtooth wave lookup.
   *
   * @param   x     Phase in [0, 1.0].
   * @param   idx   Wave index in [0,6].
   * @return        Wave sample.
   */
  __fast_inline float osc_bl_sawf(float x, uint8_t idx) {
    const float p = x - (uint32_t)x;
    
    const float x0f = 2.f * p * k_wt_saw_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    uint32_t x0 = x0p, x1 = x0p+1;
    float sign = 1.f;
    if (x0p >= k_wt_saw_size) {
      x0 = k_wt_saw_size - (x0p & k_wt_saw_mask);
      x1 = x0 - 1;
      sign = -1.f;
    }
    const float *wt = &wt_saw_lut_f[idx*k_wt_saw_lut_size];
    const float y0 = linintf(x0f - x0p, wt[x0], wt[x1]);
    return sign*y0;
  }

  /**
   * Band-limited sawtooth wave lookup. (interpolated version)
   *
   * @param   x     Phase in [0, 1.0].
   * @param   idx   Fractional wave index in [0,6].
   * @return        Wave sample.
   */
  __fast_inline float osc_bl2_sawf(float x, float idx) {
    const float p = x - (uint32_t)x;
    
    const float x0f = 2.f * p * k_wt_saw_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    uint32_t x0 = x0p, x1 = x0p+1;
    float sign = 1.f;
    if (x0p >= k_wt_saw_size) {
      x0 = k_wt_saw_size - (x0p & k_wt_saw_mask);
      x1 = x0 - 1;
      sign = -1.f;
    }
    const float *wt = &wt_saw_lut_f[(uint16_t)idx*k_wt_saw_lut_size];
    const float fr = x0f - x0p;
    const float y0 = sign * linintf(fr, wt[x0], wt[x1]);

    wt += k_wt_saw_lut_size;
    const float y1 = sign * linintf(fr, wt[x0], wt[x1]);
    
    return linintf((idx - (uint8_t)idx), y0, y1);
  } 

  /**
   * Get band-limited sawtooth wave index for note.
   *
   * @param note Fractional note in [0-151] range.
   * @return     Corresponding band-limited wave fractional index in [0-6].
   */
  float _osc_bl_saw_idx(float note);

  __fast_inline float osc_bl_saw_idx(float note) {
    return _osc_bl_saw_idx(note);
  }
  
  
  /** @} */

  /**
   * @name   Band-limited square half-waves
   * @note   Wrap interpolation to zero for 0.5, negate and reverse phase for >= 0.5
   * @{ 
   */
  
#define k_wt_sqr_size_exp      7
#define k_wt_sqr_size          (1U<<k_wt_sqr_size_exp)
#define k_wt_sqr_u32shift      (24)
#define k_wt_sqr_frrecip       (5.96046447753906e-008f) // 1/(1<<24)    
#define k_wt_sqr_mask          (k_wt_sqr_size-1)
#define k_wt_sqr_lut_size      (k_wt_sqr_size+1)
#define k_wt_sqr_notes_cnt     7
#define k_wt_sqr_lut_tsize     (k_wt_sqr_notes_cnt * k_wt_sqr_lut_size)

  extern const uint8_t wt_sqr_notes[k_wt_sqr_notes_cnt];
  extern const float wt_sqr_lut_f[k_wt_sqr_lut_tsize];

  /**
   * Square wave lookup.
   *
   * @param   x  Phase in [0, 1.0].
   * @return     Wave sample.
   * @note Not checking input, caller responsible for bounding x.
   */
  __fast_inline float osc_sqrf(float x) {
    const float p = x - (uint32_t)x;
    
    const float x0f = 2.f * p * k_wt_sqr_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    uint32_t x0 = x0p, x1 = x0p+1;
    float sign = 1.f;
    if (x0p >= k_wt_sqr_size) {
      x0 = k_wt_sqr_size - (x0p & k_wt_sqr_mask);
      x1 = x0 - 1;
      sign = -1.f;
    }
    
    const float y0 = linintf(x0f - x0p, wt_sqr_lut_f[x0], wt_sqr_lut_f[x1]);
    return sign*y0;
  }

  /**
   * Band-limited square wave lookup.
   *
   * @param   x     Phase in [0, 1.0].
   * @param   idx   Wave index in [0,6].
   * @return        Wave sample.
   * @note Not checking input, caller responsible for bounding x and idx.
   */
  __fast_inline float osc_bl_sqrf(float x, uint8_t idx) {
    const float p = x - (uint32_t)x;
    
    const float x0f = 2.f * p * k_wt_sqr_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    uint32_t x0 = x0p, x1 = x0p+1;
    float sign = 1.f;
    if (x0p >= k_wt_sqr_size) {
      x0 = k_wt_sqr_size - (x0p & k_wt_sqr_mask);
      x1 = x0 - 1;
      sign = -1.f;
    }
    const float *wt = &wt_sqr_lut_f[idx*k_wt_sqr_lut_size];
    const float y0 = linintf(x0f - x0p, wt[x0], wt[x1]);
    return sign*y0;
  }

  /**
   * Band-limited square wave lookup. (interpolated version).
   *
   * @param   x     Phase in [0, 1.0].
   * @param   idx   Fractional wave index in [0,6].
   * @return        Wave sample.
   * @note Not checking input, caller responsible for bounding x and idx.
   */
  __fast_inline float osc_bl2_sqrf(float x, float idx) {
    const float p = x - (uint32_t)x;
    
    const float x0f = 2.f * p * k_wt_sqr_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    uint32_t x0 = x0p, x1 = x0p+1;
    float sign = 1.f;
    if (x0p >= k_wt_sqr_size) {
      x0 = k_wt_sqr_size - (x0p & k_wt_sqr_mask);
      x1 = x0 - 1;
      sign = -1.f;
    }
    const float *wt = &wt_sqr_lut_f[(uint16_t)idx*k_wt_sqr_lut_size];
    const float fr = x0f - x0p;
    const float y0 = sign * linintf(fr, wt[x0], wt[x1]);

    wt += k_wt_sqr_lut_size;
    const float y1 = sign * linintf(fr, wt[x0], wt[x1]);
    
    return linintf((idx - (uint8_t)idx), y0, y1);
  }
  
  /**
   * Get band-limited square wave index for note.
   *
   * @param note Fractional note in [0-151] range.
   * @return     Corresponding band-limited wave fractional index in [0-6].
   */
  float _osc_bl_sqr_idx(float note);

  __fast_inline float osc_bl_sqr_idx(float note) {
    return _osc_bl_sqr_idx(note);
  }
  
  /** @} */

  /**
   * @name   Band-limited parabolic half-waves.
   * @note   Wrap interpolation to zero for 0.5, negate and reverse phase for >= 0.5. Careful to negate index 0 when wrapping.
   *
   * @{ 
   */
  
#define k_wt_par_size_exp      7
#define k_wt_par_size          (1U<<k_wt_par_size_exp)
#define k_wt_par_u32shift      (24)
#define k_wt_par_frrecip       (5.96046447753906e-008f) // 1/(1<<24)  
#define k_wt_par_mask          (k_wt_par_size-1)
#define k_wt_par_lut_size      (k_wt_par_size+1)
#define k_wt_par_notes_cnt     7
#define k_wt_par_lut_tsize     (k_wt_par_notes_cnt * k_wt_par_lut_size)

  extern const uint8_t wt_par_notes[k_wt_par_notes_cnt];
  extern const float wt_par_lut_f[k_wt_par_lut_tsize];

  /**
   * Parabolic wave lookup.
   *
   * @param   x  Phase in [0, 1.0].
   * @return     Wave sample.
   * @note Not checking input, caller responsible for bounding x.
   */
  __fast_inline float osc_parf(float x) {
    const float p = x - (uint32_t)x;

    const float x0f = 2.f * p * k_wt_par_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    const uint32_t x0 = (x0p<=k_wt_par_size) ? x0p : (k_wt_par_size - (x0p & k_wt_par_mask));
    const uint32_t x1 = (x0p<(k_wt_par_size-1)) ? (x0 + 1) & k_wt_par_mask : (x0p >= k_wt_par_size) ? (x0 - 1) & k_wt_par_mask : (x0 + 1);
    const float y0 = linintf(x0f - x0p, wt_par_lut_f[x0], wt_par_lut_f[x1]);
    return y0;
  }
  
  /**
   * Band-limited parabolic wave lookup.
   *
   * @param   x     Phase in [0, 1.0].
   * @param   idx   Wave index in [0,6].
   * @return        Wave sample.
   * @note Not checking input, caller responsible for bounding x and idx.
   */
  __fast_inline float osc_bl_parf(float x, uint8_t idx) {
    const float p = x - (uint32_t)x;

    const float x0f = 2.f * p * k_wt_par_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    const uint32_t x0 = (x0p<=k_wt_par_size) ? x0p : (k_wt_par_size - (x0p & k_wt_par_mask));
    const uint32_t x1 = (x0p<(k_wt_par_size-1)) ? (x0 + 1) & k_wt_par_mask : (x0p >= k_wt_par_size) ? (x0 - 1) & k_wt_par_mask : (x0 + 1);

    const float *wt = &wt_par_lut_f[idx*k_wt_par_lut_size];
    const float y0 = linintf(x0f - x0p, wt[x0], wt[x1]);
    return y0;
  }

  /**
   * Band-limited parabolic wave lookup. (interpolated version)
   *
   * @param   x     Phase in [0, 1.0].
   * @param   idx   Fractional wave index in [0,6].
   * @return        Wave sample.
   * @note Not checking input, caller responsible for bounding x and idx.
   */
  __fast_inline float osc_bl2_parf(float x, float idx) {
    const float p = x - (uint32_t)x;

    const float x0f = 2.f * p * k_wt_par_size;
    const uint32_t x0p = (uint32_t)x0f;
    
    uint32_t x0 = (x0p<=k_wt_par_size) ? x0p : (k_wt_par_size - (x0p & k_wt_par_mask));
    uint32_t x1 = (x0p<(k_wt_par_size-1)) ? (x0 + 1) & k_wt_par_mask : (x0p >= k_wt_par_size) ? (x0 - 1) & k_wt_par_mask : (x0 + 1);

    const float *wt = &wt_par_lut_f[(uint16_t)idx*k_wt_par_lut_size];
    const float fr = x0f - x0p;
    const float y0 = linintf(fr, wt[x0], wt[x1]);

    wt += k_wt_par_lut_size;
    
    const float y1 = linintf(fr, wt[x0], wt[x1]);

    return linintf((idx - (uint8_t)idx), y0, y1);
  }
  
  /**
   * Get band-limited parabolic wave index for note.
   *
   * @param note Fractional note in [0-151] range.
   * @return     Corresponding band-limited wave fractional index in [0-6].
   */
  float _osc_bl_par_idx(float note);

  __fast_inline float osc_bl_par_idx(float note) {
    return _osc_bl_par_idx(note);
  }
  
  /** @} */
  
  /*===========================================================================*/
  /* Waves                                                                     */
  /*===========================================================================*/
  /**
   * @name   Wave banks.
   *
   * Banks are organized in categories from A to F in order of increasing harmonic components.
   *
   * @{ 
   */

#define k_waves_size_exp   (7)
#define k_waves_size       (1U<<k_waves_size_exp)
#define k_waves_u32shift   (24)
#define k_waves_frrecip    (5.96046447753906e-008f) // 1/(1<<24)
#define k_waves_mask       (k_waves_size-1)
#define k_waves_lut_size   (k_waves_size+1)
  
#define k_waves_a_cnt       16
  
  extern const float * const wavesA[k_waves_a_cnt];

#define k_waves_b_cnt       16
  
  extern const float * const wavesB[k_waves_b_cnt];

#define k_waves_c_cnt       14
  
  extern const float * const wavesC[k_waves_c_cnt];

#define k_waves_d_cnt       13
  
  extern const float * const wavesD[k_waves_d_cnt];

#define k_waves_e_cnt       15
  
  extern const float * const wavesE[k_waves_e_cnt];

#define k_waves_f_cnt       16
  
  extern const float * const wavesF[k_waves_f_cnt];
  
  static inline __attribute__((always_inline, optimize("Ofast")))
  float osc_wave_scanf(const float *w, float x) {
    const float p = x - (uint32_t)x;
    const float x0f = p * k_waves_size;
    const uint32_t x0 = ((uint32_t)x0f) & k_waves_mask;
    const uint32_t x1 = (x0 + 1) & k_waves_mask;
    return linintf(x0f - (uint32_t)x0f, w[x0], w[x1]);
  }

  static inline __attribute__((always_inline, optimize("Ofast")))
  float osc_wave_scanuf(const float *w, uint32_t x) {
    const uint32_t x0 = (x>>k_waves_u32shift);
    const uint32_t x1 = (x0 + 1) & k_waves_mask;
    const float fr = k_waves_frrecip * (float)(x & ((1U<<k_waves_u32shift)-1));
    return linintf(fr, w[x0], w[x1]);
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
  __fast_inline float osc_logf(float x) {
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
  __fast_inline float osc_tanpif(float x) {
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
  __fast_inline float osc_sqrtm2logf(float x) {
    const float idxf = (x-k_sqrtm2log_base) * k_sqrtm2log_range_recip * k_sqrtm2log_size;
    const uint32_t idx = (uint32_t)idxf;
    const float y0 = sqrtm2log_lut_f[idx];
    const float y1 = sqrtm2log_lut_f[idx+1];
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
  __fast_inline float osc_softclipf(const float c, float x)
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
  __fast_inline float osc_sat_cubicf(float x) {
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
  __fast_inline float osc_sat_schetzenf(float x) {
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
  __fast_inline float osc_bitresf(float x) {
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
  uint32_t _osc_rand(void);

  __fast_inline uint32_t osc_rand(void) {
    return _osc_rand();
  }
  
  /**
   * Gaussian white noise
   *
   * @return     Value in [-1.0, 1.0].
   */
  float _osc_white(void);

  __fast_inline float osc_white(void) {
    return _osc_white();
  }
  
  /** @} */

  /** @} */
  
#ifdef __cplusplus
}
#endif 


#endif // __osc_api_h

/** @} @} */
