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
 * @file    common_fixed_math.h
 * @brief   Fixed Point Math Utilities.
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_common_fixed_math Fixed-Point Math
 * @{
 *
 */

 #ifndef __common_fixed_math_h
 #define __common_fixed_math_h
 
 #include <stdint.h>
 
 // TODO: add fill-in implementations for intel builds
 
 /*===========================================================================*/
 /* Data Types and Conversions.                                               */
 /*===========================================================================*/
 
 /**
  * @name    Fixed point Types
  * @{
  */
 
 typedef int8_t  q7_t;
 typedef int8_t  q7_0_t;
 
 typedef uint8_t q8_t;
 typedef uint8_t q8_0_t;
 
 typedef int16_t q15_t;
 typedef int16_t q1_14_t;
 typedef int16_t q2_13_t;
 typedef int16_t q3_12_t;
 typedef int16_t q4_11_t;
 typedef int16_t q5_10_t;
 typedef int16_t q6_9_t;
 typedef int16_t q7_8_t;
 typedef int16_t q8_7_t;
 typedef int16_t q13_2_t;
 typedef int16_t q15_0_t;
 
 typedef uint16_t uq16_t;
 typedef uint16_t uq16_0_t;
 
 typedef int32_t q23_t;
 
 typedef int32_t q31_t;
 typedef int32_t q1_30_t;
 typedef int32_t q2_29_t;
 typedef int32_t q3_28_t;
 typedef int32_t q4_27_t;
 typedef int32_t q5_26_t;
 typedef int32_t q6_25_t;
 typedef int32_t q7_24_t;
 typedef int32_t q8_23_t;
 typedef int32_t q15_16_t;
 typedef int32_t q16_15_t;
 typedef int32_t q17_14_t;
 typedef int32_t q18_13_t;
 typedef int32_t q19_12_t;
 typedef int32_t q20_11_t;
 typedef int32_t q21_10_t;
 typedef int32_t q22_9_t;
 typedef int32_t q23_8_t;
 typedef int32_t q31_0_t;
 
 typedef uint32_t uq32_t;
 typedef uint32_t uq14_18_t;
 typedef uint32_t uq32_0_t;
 
 typedef int64_t q63_t;
 typedef int64_t q1_62_t;
 typedef int64_t q2_61_t;
 typedef int64_t q3_60_t;
 typedef int64_t q4_59_t;
 typedef int64_t q5_58_t;
 typedef int64_t q6_57_t;
 typedef int64_t q7_56_t;
 typedef int64_t q8_55_t;
 typedef int64_t q16_47_t;
 typedef int64_t q23_40_t;
 typedef int64_t q31_32_t;
 typedef int64_t q63_0_t;
 
 typedef uint64_t uq64_t;
 typedef uint64_t uq64_0_t;
 
 /** @} */
 
 /**
  * @name    Fixed-Point and Floating Point Type Conversions
  * @note    Should get properly compiled to VCVT instructions on ARM Cortex-M4
  * @{
  */
  
 #define q7_to_f32_c 0.007874015748031496f
 #define uq8_to_f32_c 0.00392156862745098f
 #define q9_to_f32_c 0.001953125f
 #define uq9_to_f32_c 0.0019569471624266144f
 #define uq10_to_f32_c 0.0009775171065493646f
 #define q15_to_f32_c 3.05175781250000e-005f
 #define uq16_to_f32_c 1.5259021896696422e-05f
 #define q31_to_f32_c 4.65661287307739e-010f
 #define uq32_to_f32_c 2.3283064370807974e-10f

 #define q7_to_f32(q) ((float)(q)*q7_to_f32_c)
 #define uq8_to_f32(q) ((float)(q)*uq8_to_f32_c)
 #define q9_to_f32(q) ((float)(q)*q9_to_f32_c)
 #define uq9_to_f32(q) ((float)(q)*uq9_to_f32_c)
 #define uq10_to_f32(q) ((float)(q)*uq10_to_f32_c)
 #define q15_to_f32(q) ((float)(q)*q15_to_f32_c)
 #define uq16_to_f32(q) ((float)(q)*uq16_to_f32_c)
 #define q31_to_f32(q) ((float)(q)*q31_to_f32_c)
 #define uq32_to_f32(q) ((float)(q)*uq32_to_f32_c)

 #define f32_to_q7(f) ((q8_t)((float)(f) * (float)0x7F))  // careful, wont saturate for close to 1.f
 #define f32_to_uq8(f) ((uq8_t)((float)(f) * (float)0xFF))
 #define f32_to_q9(f) ((q9_t)((float)(f) * (float)0x1FF))  // careful, wont saturate for close to 1.f
 #define f32_to_uq9(f) ((uq9_t)((float)(f) * (float)0x1FF))
 #define f32_to_uq10(f) ((uq10_t)((float)(f) * (float)0x3FF))
 #define f32_to_q15(f) ((q15_t)((float)(f) * (float)0x7FFF))  // careful, wont saturate for close to 1.f
 #define f32_to_uq16(f) ((uq16_t)((float)(f) * (float)0xFFFF))
 #define f32_to_q31(f) ((q31_t)((float)(f) * (float)0x7FFFFFFF))
 #define f32_to_uq32(f) ((uq32_t)((float)(f) * (float)0xFFFFFFFF))
 
 /** @} */
 
 /*===========================================================================*/
 /* Constants.                                                                */
 /*===========================================================================*/
 
 /**
  * @name    Fixed point Constants
  * @{
  */
 
 #define M_HALPI_Q1_14 0x6488
 #define M_HALPI_Q1_30 0x6487ED51
 #define M_PI_Q2_13    0x6488
 #define M_PI_Q2_29    0x6487ED51
 #define M_1OVERPI_Q15 0x28BE
 #define M_1OVERPI_Q31 0x28BE60DC
 #define M_TWOPI_Q3_12 0x6488
 #define M_TWOPI_Q3_28 0x6487ED51
 
 #define M_1OVER48K_Q31 0x0000AEC3 // 1/48000
 #define M_1OVER44K_Q31 0x0000BE38 // 1/44100
 #define M_1OVER22K_Q31 0x00017C70 // 1/22050
 
 /** @} */ 
 
 #endif // __common_fixed_math_h
 
 /** @} @} */
 