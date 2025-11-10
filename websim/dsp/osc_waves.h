/*
 * File: osc_waves.h
 *
 * User-accessible waves
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017-2022 (c) Korg
 *
 */

/**
 * @file    osc_waves.h
 * @brief   User accessible waves
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __osc_waves_h
#define __osc_waves_h

#include <stdint.h>
// #include "dsp/wavetable.h"

#ifdef __cplusplus
extern "C" {
#endif

#define k_waves_size        128
#define k_waves_mask       (k_waves_size-1)
#define k_waves_lut_size   (k_waves_size+1)
#define k_waves_period      k_wt_period_full
#define k_waves_notes_cnt   1
#define k_waves_lut_tsize  (k_wt_waves_notes_cnt * k_wt_waves_lut_size)

#define k_waves_a_cnt       16 //34
  
  extern const float * const wavesA[k_waves_a_cnt];

#define k_waves_b_cnt       16 //18
  
  extern const float * const wavesB[k_waves_b_cnt];

#define k_waves_c_cnt       14
  
  extern const float * const wavesC[k_waves_c_cnt];

#define k_waves_d_cnt       13
  
  extern const float * const wavesD[k_waves_d_cnt];

#define k_waves_e_cnt       15 // 17
  
  extern const float * const wavesE[k_waves_e_cnt];

#define k_waves_f_cnt       16 // 20
  
  extern const float * const wavesF[k_waves_f_cnt];
  
  
#ifdef __cplusplus
}
#endif

  
#endif // __waves_h

/** @} */
