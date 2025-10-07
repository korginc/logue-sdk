/*
 * File: tanpi_lut.h
 *
 * tan(pi*x)
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017 (c) Korg
 *
 */

/**
 * @file    tanpi_lut.h
 * @brief   tan(pi*x)
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __tanpi_lut_h
#define __tanpi_lut_h

#include "utils/common_fixed_math.h"
#include "utils/common_float_math.h"
#include "attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define k_tanpi_size_exp         (8)
#define k_tanpi_size             (1U<<k_tanpi_size_exp)
#define k_tanpi_mask             (k_tanpi_size-1)
#define k_tanpi_range_recip      (2.04081632653061f) // 1/0.49
#define k_tanpi_lut_size         (k_tanpi_size+1)
  
extern const float tanpi_lut_f[k_tanpi_lut_size];

/**
* Lookup value of tan(pi*x) in [0.0001, 0.49] range.
*
* @param   x  Value in [0.0001, 0.49].
* @return     Result of tan(pi*x).
* @note Not checking input, caller responsible for bounding x.
*/
static fast_inline float si_tanpif(float x) 
{
   const float idxf = x * k_tanpi_range_recip * k_tanpi_size;
   const uint32_t idx = (uint32_t)idxf;
   const float y0 = tanpi_lut_f[idx];
   const float y1 = tanpi_lut_f[idx+1];
   return linintf(idxf - idx, y0, y1);
}

#ifdef __cplusplus
}
#endif

#endif  // __tanpi_lut_h

/** @} */
