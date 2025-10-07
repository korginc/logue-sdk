/*
 * File: wavetable_lut.h
 *
 * Basic Wavetables
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017-2022 (c) Korg
 *
 */

/**
 * @file    wavetable_lut.h
 * @brief   Basic Wavetables.
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __wavetable_lut_h
#define __wavetable_lut_h

// #include "synths_common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * @name   Sine half-wave
   * @note   Wrap and negate for phase >= 0.5
   * @{ 
   */

#define k_wt_sine_size_exp     7
#define k_wt_sine_size         (1U<<k_wt_sine_size_exp)
#define k_wt_sine_mask         (k_wt_sine_size-1)
#define k_wt_sine_lut_size     (k_wt_sine_size+1)
#define k_wt_sine_period       k_wt_period_half
#define k_wt_sine_scan_mode    k_wt_scan_mode_wrap_invert
#define k_wt_sine_notes_cnt    1
#define k_wt_sine_lut_tsize    (k_wt_sine_notes_cnt * k_wt_sine_lut_size)
extern const float wt_sine_lut_f[k_wt_sine_lut_tsize];

/** @} */

/**
 * @name   Band-limited sawtooth half-waves
 * @note   Wrap interpolation to zero for 0.5, negate and reverse phase for >= 0.5
 * @{ 
 */

#define k_wt_saw_size_exp      7
#define k_wt_saw_size          (1U<<k_wt_saw_size_exp)
#define k_wt_saw_mask          (k_wt_saw_size-1)
#define k_wt_saw_lut_size      (k_wt_saw_size+1)
#define k_wt_saw_period        k_wt_period_half
#define k_wt_saw_scan_mode     k_wt_scan_mode_rev_invert
#define k_wt_saw_notes_cnt     7
#define k_wt_saw_lut_tsize     (k_wt_saw_notes_cnt * k_wt_saw_lut_size)
extern const uint8_t wt_saw_notes[k_wt_saw_notes_cnt];
extern const float wt_saw_lut_f[k_wt_saw_lut_tsize];

/** @} */

/**
 * @name   Band-limited square half-waves
 * @note   Wrap interpolation to zero for 0.5, negate and reverse phase for >= 0.5
 * @{ 
 */

#define k_wt_sqr_size_exp      7
#define k_wt_sqr_size          (1U<<k_wt_sqr_size_exp)
#define k_wt_sqr_mask          (k_wt_sqr_size-1)
#define k_wt_sqr_lut_size      (k_wt_sqr_size+1)
#define k_wt_sqr_period        k_wt_period_half
#define k_wt_sqr_scan_mode     k_wt_scan_mode_rev_invert
#define k_wt_sqr_notes_cnt     7
#define k_wt_sqr_lut_tsize     (k_wt_sqr_notes_cnt * k_wt_sqr_lut_size)
extern const uint8_t wt_sqr_notes[k_wt_sqr_notes_cnt];
extern const float wt_sqr_lut_f[k_wt_sqr_lut_tsize];

/** @} */

/**
 * @name   Band-limited parabolic half-waves
 * @note   Wrap interpolation to zero for 0.5, negate and reverse phase for >= 0.5. Careful to negate index 0 when wrapping
 *
 * @{ 
 */

#define k_wt_par_size_exp      7
#define k_wt_par_size          (1U<<k_wt_par_size_exp)
#define k_wt_par_mask          (k_wt_par_size-1)
#define k_wt_par_lut_size      (k_wt_par_size+1)
#define k_wt_par_period        k_wt_period_half
#define k_wt_par_scan_mode     k_wt_scan_mode_rev
#define k_wt_par_notes_cnt     7
#define k_wt_par_lut_tsize     (k_wt_par_notes_cnt * k_wt_par_lut_size)
extern const uint8_t wt_par_notes[k_wt_par_notes_cnt];
extern const float wt_par_lut_f[k_wt_par_lut_tsize];

/** @} */

#ifdef __cplusplus
}
#endif

  
#endif // __wavetable_lut_h

/** @} */
