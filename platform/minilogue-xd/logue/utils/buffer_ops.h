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
 * @file    buffer_ops.h
 * @brief   Operations over data buffers.
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_buffer_ops Buffer Operations
 * @{
 *
 */

#ifndef __buffer_ops_h
#define __buffer_ops_h

#include "fixed_math.h"
#include "int_math.h"
#include "float_math.h"

#define REP4(expr) (expr);(expr);(expr);(expr);

/**
 * @name    Buffer format conversion
 * @{
 */

/** Buffer-wise Q31 to float conversion
 */
static inline __attribute__((optimize("Ofast"),always_inline))
void buf_q31_to_f32(const q31_t *q31,
                    float * __restrict__ flt,
                    const size_t len)
{
  const float *end = flt + ((len>>2)<<2);
  for (; flt != end; ) {
    REP4(*(flt++) = q31_to_f32(*(q31++)));
  };
  end += len & 0x3;
  for (; flt != end; ) {
    *(flt++) = q31_to_f32(*(q31++));
  }
}

/** Buffer-wise float to Q31 conversion
 */
static inline __attribute__((optimize("Ofast"),always_inline))
void buf_f32_to_q31(const float *flt,
                    q31_t * __restrict__ q31,
                    const size_t len)
{
  const float *end = flt + ((len>>2)<<2);
  for (; flt != end; ) {
    REP4(*(q31++) = f32_to_q31(*(flt++)));
  }
  end += len & 0x3;
  for (; flt != end; ) {
    *(q31++) = f32_to_q31(*(flt++));
  }
}

//** @} */

/**
 * @name    Buffer clear
 * @{
 */

/** Buffer clear (float version)
 */
static inline __attribute__((optimize("Ofast"),always_inline))
void buf_clr_f32(float * __restrict__ ptr,
                 const uint32_t len)
{
  const float *end = ptr + ((len>>2)<<2);
  for (; ptr != end; ) {
    REP4(*(ptr++) = 0);
  }
  end += len & 0x3;
  for (; ptr != end; ) {
    *(ptr++) = 0;
  }
}

/** Buffer clear (32bit unsigned integer version).
 */
static inline __attribute__((optimize("Ofast"),always_inline))
void buf_clr_u32(uint32_t * __restrict__ ptr,
                 const size_t len)
{
  const uint32_t *end = ptr + ((len>>2)<<2);
  for (; ptr != end; ) {
    REP4(*(ptr++) = 0);
  }
  end += len & 0x3;
  for (; ptr != end; ) {
    *(ptr++) = 0;
  }
}

//** @} */

/**
 * @name    Buffer copy
 * @{
 */

/** Buffer copy (float version).
 */
static inline __attribute__((optimize("Ofast"),always_inline))
void buf_cpy_f32(const float *src,
                 float * __restrict__ dst,
                 const size_t len)
{
  const float *end = src + ((len>>2)<<2);
  for (; src != end; ) {
    REP4(*(dst++) = *(src++));
  }
  end += len & 0x3;
  for (; src != end; ) {
    *(dst++) = *(src++);
  }
}

/** Buffer copy (32bit unsigned integer version).
 */
static inline __attribute__((optimize("Ofast"),always_inline))
void buf_cpy_u32(const uint32_t *src,
                 uint32_t * __restrict__ dst,
                 const size_t len)
{
  const uint32_t *end = src + ((len>>2)<<2);
  for (; src != end; ) {
    REP4(*(dst++) = *(src++));
  }
  end += len & 0x3;
  for (; src != end; ) {
    *(dst++) = *(src++);
  }
}

//** @} */

#endif // __buffer_ops_h

/** @} @} */
