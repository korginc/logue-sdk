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
 * @file    int_math.h
 * @brief   Integer Math Utilities.
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_int_math Integer Math
 * @{
 *
 */

#ifndef __int_math_h
#define __int_math_h

#include <stdint.h>

/*===========================================================================*/
/* Clipping Operations.                                                      */
/*===========================================================================*/

/**
 * @name    Clipping Operations
 * @{
 */

/** Clip upper bound of signed integer x to m (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
int32_t clipmaxi32(const int32_t x, const int32_t m) {
  return (((x)>=m)?m:(x));
}

/** Clip lower bound of signed integer x to m (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
int32_t clipmini32(const int32_t  m, const int32_t x) {
  return (((x)<=m)?m:(x));
}

/** Clip signe integer x between min and max (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
int32_t clipminmaxi32(const int32_t min, const int32_t x, const int32_t max) {
  return (((x)>=max)?max:((x)<=min)?min:(x));
}

/** Clip upper bound of unsigned integer x to m (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
uint32_t clipmaxu32(const uint32_t x, const uint32_t m) {
  return (((x)>=m)?m:(x));
}

/** Clip lower bound of unsigned integer x to m (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
uint32_t clipminu32(const uint32_t  m, const uint32_t x) {
  return (((x)<=m)?m:(x));
}

/** Clip unsigned integer x between min and max (inclusive)
 */
static inline __attribute__((optimize("Ofast"), always_inline))
uint32_t clipminmaxu32(const uint32_t min, const uint32_t x, const uint32_t max) {
  return (((x)>=max)?max:((x)<=min)?min:(x));
}


/** @} */

/*===========================================================================*/
/* Power of 2s.                                                              */
/*===========================================================================*/

/**
 * @name    Power of 2s
 * @{
 */

/** Compute next power of 2 greater than x
 */
static inline __attribute__((always_inline))
uint32_t nextpow2_u32(uint32_t x) {
  x--;
  x |= x>>1; x |= x>>2;
  x |= x>>4; x |= x>>8;
  x |= x>>16;
  return ++x;
}

/** Check if x is a power of 2
 */
static inline __attribute__((always_inline))
uint8_t ispow2_u32(const uint32_t x) {
  return x && !(x & (x-1));
}

/** @} */

#endif // __int_math_h

/** @} @} */
