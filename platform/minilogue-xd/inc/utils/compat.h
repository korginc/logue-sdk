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
 * @file    compat.h
 * @brief   Compatibility drop-in replacements for some ARM Cortex-M intrinsics
 *
 * @addtogroup UTILS
 * @{
 */

#ifndef __compat_h
#define __compat_h

#include <stdint.h>

#define COMPAT_INTRINSICS

// ARM Cortex-M4 Core Intrinsics ------------------------------------------------------------
// See http://www.keil.com/pack/doc/cmsis/Core/html/group__intrinsic__CPU__gr.html

static inline __attribute__((always_inline)) void __BKPT(uint8_t value) { (void)value; }
static inline __attribute__((always_inline)) void __CLREX(void) { }

static inline __attribute__((always_inline)) uint32_t __CLZ(int32_t data) {
  uint32_t count = 0;
  uint32_t mask = 0x80000000;
  
  while((data & mask) == 0) {
    count += 1u;
    mask = mask >> 1u;
  }
  
  return count;
}

static inline __attribute__((always_inline)) void __DMB(void) { }
static inline __attribute__((always_inline)) void __DSB(void) { }
static inline __attribute__((always_inline)) void __ISB(void) { }
static inline __attribute__((always_inline)) void __NOP(void) { }

static inline __attribute__((always_inline)) uint32_t __RBIT(uint32_t v) {
  uint32_t r = 0;
  for (uint8_t i=32; i != 0; --i) {
    r |= (v & 0x1);
    r <<= 1;
    v >>= 1;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __REV(uint32_t v) {
  uint32_t r = 0;
  for (uint8_t i=4; i != 0; --i) {
    r |= (v & 0xFF);
    r <<= 8;
    v >>= 8;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __REV16(uint32_t v) {
  uint32_t r = 0;
  for (uint8_t i=2; i != 0; --i) {
    r |= (v & 0xFF) | (v & 0xFF0000);
    r <<= 8;
    v >>= 8;
  }
  return r;
}

static inline __attribute__((always_inline)) int16_t __REVSH(int16_t v) {
  uint16_t uv = (uint16_t)v;
  uint16_t r = 0;
  for (uint8_t i=2; i != 0; --i) {
    r |= (uv & 0xFF) | (uv & 0xFF0000);
    r <<= 8;
    uv >>= 8;
  }
  return (int16_t)r;
}

static inline __attribute__((always_inline)) uint32_t __ROR(uint32_t v, uint32_t shift) {
  return (v >> shift) | (v << (32-shift));
}

static inline __attribute__((always_inline)) uint32_t __RRX(uint32_t v) {
  return (v >> 1) | ((v & 0x1)<<31);
}

static inline __attribute__((always_inline)) void __SEV(void) { }

static inline __attribute__((always_inline)) int32_t __SSAT(int32_t v, uint32_t sat) {
  const int32_t ref = 1ULL << (sat-1);
  return (v >= ref) ? (ref-1) : (v <= -ref) ? -ref : v;  
}

static inline __attribute__((always_inline)) uint32_t __USAT(uint32_t v, uint32_t sat) {
  const uint32_t ref = 1ULL << sat;
  return (v >= ref) ? ref : v;  
}

static inline __attribute__((always_inline)) void __WFE(void) { }
static inline __attribute__((always_inline)) void __WFI(void) { }

#define bkpt   __BKPT
#define clrex  __CLREX
#define clz    __CLZ
#define dmb    __DMB
#define dsb    __DSB
#define isb    __ISB
#define nop    __NOP
#define rbit   __RBIT
#define rev    __REV
#define rev16  __REV16
#define revsh  __REVSH
#define ror    __ROR
#define rrx    __RRX
#define sev    __SEV
#define ssat   __SSAT
#define usat   __USAT
#define wfe    __WFE
#define wfi    __WFI

#define apsr() 
#define apsr_clr(m) 

// ARM Cortex-M4 SIMD Intrinsics ------------------------------------------------------------
// See http://www.keil.com/pack/doc/cmsis/Core/html/group__intrinsic__SIMD__gr.html
//   and http://www.keil.com/support/man/docs/ARMASM/armasm_dom1361289850039.htm

#define __SIMD32_TYPE int32_t
#define simd32_t __SIMD32_TYPE

// 8-bit Arithmetic ----------------------------------------------

static inline __attribute__((always_inline)) uint32_t __SADD8(uint32_t a, uint32_t b) {
  uint32_t r = (uint8_t)((int8_t)(a & 0xFF) + (int8_t)(b & 0xFF)) & 0xFF;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>8) & 0xFF) + (int8_t)((b>>8) & 0xFF))) & 0xFF)<<8;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>16) & 0xFF) + (int8_t)((b>>16) & 0xFF))) & 0xFF)<<16;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>24) & 0xFF) + (int8_t)((b>>24) & 0xFF))) & 0xFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __QADD8(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (uint8_t i = 4; i != 0; --i) {
    int32_t t = (int8_t)(a & 0xFF) + (int8_t)(b & 0xFF);
    t = (t >= (1<<7)) ? (1<<7)-1 : (t <= -(1<<7)) ? -(1<<7) : t;
    r |= (uint32_t)(t & 0xFF) << ((4-i)*8);
    a >>= 8;
    b >>= 8;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SHADD8(uint32_t a, uint32_t b) {
  uint32_t r = ((uint8_t)((int8_t)(a & 0xFF) + (int8_t)(b & 0xFF))>>1) & 0xFF;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>8) & 0xFF) + (int8_t)((b>>8) & 0xFF))>>1) & 0xFF)<<8;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>16) & 0xFF) + (int8_t)((b>>16) & 0xFF))>>1) & 0xFF)<<16;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>24) & 0xFF) + (int8_t)((b>>24) & 0xFF))>>1) & 0xFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UADD8(uint32_t a, uint32_t b) {
  uint32_t r = ((uint8_t)(a & 0xFF) + (uint8_t)(b & 0xFF)) & 0xFF;
  r |= ((uint32_t)((uint8_t)((a>>8) & 0xFF) + (uint8_t)((b>>8) & 0xFF)) & 0xFF)<<8;
  r |= ((uint32_t)((uint8_t)((a>>16) & 0xFF) + (uint8_t)((b>>16) & 0xFF)) & 0xFF)<<16;
  r |= ((uint32_t)((uint8_t)((a>>24) & 0xFF) + (uint8_t)((b>>24) & 0xFF)) & 0xFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UQADD8(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (uint8_t i = 4; i != 0; --i) {
    uint32_t t = (uint8_t)(a & 0xFF) + (uint8_t)(b & 0xFF);
    t = (t >= (1U<<8)) ? (1U<<8)-1 : t;
    r |= (uint32_t)(t & 0xFF) << ((4-i)*8);
    a >>= 8;
    b >>= 8;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UHADD8(uint32_t a, uint32_t b) {
  uint32_t r = (((uint8_t)(a & 0xFF) + (uint8_t)(b & 0xFF))>>1) & 0xFF;
  r |= ((uint32_t)(((uint8_t)((a>>8) & 0xFF) + (uint8_t)((b>>8) & 0xFF))>>1) & 0xFF)<<8;
  r |= ((uint32_t)(((uint8_t)((a>>16) & 0xFF) + (uint8_t)((b>>16) & 0xFF))>>1) & 0xFF)<<16;
  r |= ((uint32_t)(((uint8_t)((a>>24) & 0xFF) + (uint8_t)((b>>24) & 0xFF))>>1) & 0xFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SSUB8(uint32_t a, uint32_t b) {
  uint32_t r = (uint8_t)((int8_t)(a & 0xFF) - (int8_t)(b & 0xFF)) & 0xFF;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>8) & 0xFF) - (int8_t)((b>>8) & 0xFF))) & 0xFF)<<8;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>16) & 0xFF) - (int8_t)((b>>16) & 0xFF))) & 0xFF)<<16;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>24) & 0xFF) - (int8_t)((b>>24) & 0xFF))) & 0xFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __QSUB8(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (uint8_t i = 4; i != 0; --i) {
    int32_t t = (int8_t)(a & 0xFF) - (int8_t)(b & 0xFF);
    t = (t >= (1<<7)) ? (1<<7)-1 : (t <= -(1<<7)) ? -(1<<7) : t;
    r |= (uint32_t)(t & 0xFF) << ((4-i)*8);
    a >>= 8;
    b >>= 8;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SHSUB8(uint32_t a, uint32_t b) {
  uint32_t r = ((uint8_t)((int8_t)(a & 0xFF) - (int8_t)(b & 0xFF))>>1) & 0xFF;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>8) & 0xFF) - (int8_t)((b>>8) & 0xFF))>>1) & 0xFF)<<8;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>16) & 0xFF) - (int8_t)((b>>16) & 0xFF))>>1) & 0xFF)<<16;
  r |= ((uint32_t)((uint8_t)((int8_t)((a>>24) & 0xFF) - (int8_t)((b>>24) & 0xFF))>>1) & 0xFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __USUB8(uint32_t a, uint32_t b) {
  uint32_t r = ((uint8_t)(a & 0xFF) - (uint8_t)(b & 0xFF)) & 0xFF;
  r |= ((uint32_t)((uint8_t)((a>>8) & 0xFF) - (uint8_t)((b>>8) & 0xFF)) & 0xFF)<<8;
  r |= ((uint32_t)((uint8_t)((a>>16) & 0xFF) - (uint8_t)((b>>16) & 0xFF)) & 0xFF)<<16;
  r |= ((uint32_t)((uint8_t)((a>>24) & 0xFF) - (uint8_t)((b>>24) & 0xFF)) & 0xFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UQSUB8(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (uint8_t i = 4; i != 0; --i) {
    uint32_t t = (uint8_t)(a & 0xFF) - (uint8_t)(b & 0xFF);
    t = (t >= (1U<<8)) ? (1U<<8)-1 : t;
    r |= (uint32_t)(t & 0xFF) << ((4-i)*8);
    a >>= 8;
    b >>= 8;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UHSUB8(uint32_t a, uint32_t b) {
  uint32_t r = (((uint8_t)(a & 0xFF) - (uint8_t)(b & 0xFF))>>1) & 0xFF;
  r |= ((uint32_t)(((uint8_t)((a>>8) & 0xFF) - (uint8_t)((b>>8) & 0xFF))>>1) & 0xFF)<<8;
  r |= ((uint32_t)(((uint8_t)((a>>16) & 0xFF) - (uint8_t)((b>>16) & 0xFF))>>1) & 0xFF)<<16;
  r |= ((uint32_t)(((uint8_t)((a>>24) & 0xFF) - (uint8_t)((b>>24) & 0xFF))>>1) & 0xFF)<<24;
  return r;
}

#define sadd8   __SADD8
#define qadd8   __QADD8
#define shadd8  __SHADD8
#define uadd8   __UADD8
#define uqadd8  __UQADD8
#define uhadd8  __UHADD8
#define ssub8   __SSUB8
#define qsub8   __QSUB8
#define shsub8  __SHSUB8
#define usub8   __USUB8
#define uqsub8  __UQSUB8
#define uhsub8  __UHSUB8

// 16-bit Arithmetic ----------------------------------------------

static inline __attribute__((always_inline)) uint32_t __SADD16(uint32_t a, uint32_t b) {
  uint32_t r = (uint16_t)((int16_t)(a & 0xFFFF) + (int16_t)(b & 0xFFFF)) & 0xFFFF;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>16) & 0xFFFF) + (int16_t)((b>>16) & 0xFFFF))) & 0xFFFF)<<16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __QADD16(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (uint16_t i = 2; i != 0; --i) {
    int32_t t = (int16_t)(a & 0xFFFF) + (int16_t)(b & 0xFFFF);
    t = (t >= (1<<15)) ? (1<<15)-1 : (t <= -(1<<15)) ? -(1<<15) : t;
    r |= (uint32_t)(t & 0xFFFF) << ((2-i)*16);
    a >>= 16;
    b >>= 16;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SHADD16(uint32_t a, uint32_t b) {
  uint32_t r = ((uint16_t)((int16_t)(a & 0xFFFF) + (int16_t)(b & 0xFFFF))>>1) & 0xFFFF;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>8) & 0xFFFF) + (int16_t)((b>>8) & 0xFFFF))>>1) & 0xFFFF)<<8;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>16) & 0xFFFF) + (int16_t)((b>>16) & 0xFFFF))>>1) & 0xFFFF)<<16;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>24) & 0xFFFF) + (int16_t)((b>>24) & 0xFFFF))>>1) & 0xFFFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UADD16(uint32_t a, uint32_t b) {
  uint32_t r = ((uint16_t)(a & 0xFFFF) + (uint16_t)(b & 0xFFFF)) & 0xFFFF;
  r |= ((uint32_t)((uint16_t)((a>>8) & 0xFFFF) + (uint16_t)((b>>8) & 0xFFFF)) & 0xFFFF)<<8;
  r |= ((uint32_t)((uint16_t)((a>>16) & 0xFFFF) + (uint16_t)((b>>16) & 0xFFFF)) & 0xFFFF)<<16;
  r |= ((uint32_t)((uint16_t)((a>>24) & 0xFFFF) + (uint16_t)((b>>24) & 0xFFFF)) & 0xFFFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UQADD16(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (uint16_t i = 2; i != 0; --i) {
    uint32_t t = (uint16_t)(a & 0xFFFF) + (uint16_t)(b & 0xFFFF);
    t = (t >= (1U<<16)) ? (1U<<16)-1 : t;
    r |= (uint32_t)(t & 0xFFFF) << ((2-i)*16);
    a >>= 16;
    b >>= 16;
  }
  return r;
}


static inline __attribute__((always_inline)) uint32_t __UHADD16(uint32_t a, uint32_t b) {
  uint32_t r = (((uint16_t)(a & 0xFFFF) + (uint16_t)(b & 0xFFFF))>>1) & 0xFFFF;
  r |= ((uint32_t)(((uint16_t)((a>>8) & 0xFFFF) + (uint16_t)((b>>8) & 0xFFFF))>>1) & 0xFFFF)<<8;
  r |= ((uint32_t)(((uint16_t)((a>>16) & 0xFFFF) + (uint16_t)((b>>16) & 0xFFFF))>>1) & 0xFFFF)<<16;
  r |= ((uint32_t)(((uint16_t)((a>>24) & 0xFFFF) + (uint16_t)((b>>24) & 0xFFFF))>>1) & 0xFFFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SSUB16(uint32_t a, uint32_t b) {
  uint32_t r = (uint16_t)((int16_t)(a & 0xFFFF) - (int16_t)(b & 0xFFFF)) & 0xFFFF;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>8) & 0xFFFF) - (int16_t)((b>>8) & 0xFFFF))) & 0xFFFF)<<8;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>16) & 0xFFFF) - (int16_t)((b>>16) & 0xFFFF))) & 0xFFFF)<<16;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>24) & 0xFFFF) - (int16_t)((b>>24) & 0xFFFF))) & 0xFFFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __QSUB16(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (uint16_t i = 2; i != 0; --i) {
    int32_t t = (int16_t)(a & 0xFFFF) - (int16_t)(b & 0xFFFF);
    t = (t >= (1<<15)) ? (1<<15)-1 : (t <= -(1<<15)) ? -(1<<15) : t;
    r |= (uint32_t)(t & 0xFFFF) << ((2-i)*16);
    a >>= 16;
    b >>= 16;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SHSUB16(uint32_t a, uint32_t b) {
  uint32_t r = ((uint16_t)((int16_t)(a & 0xFFFF) - (int16_t)(b & 0xFFFF))>>1) & 0xFFFF;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>8) & 0xFFFF) - (int16_t)((b>>8) & 0xFFFF))>>1) & 0xFFFF)<<8;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>16) & 0xFFFF) - (int16_t)((b>>16) & 0xFFFF))>>1) & 0xFFFF)<<16;
  r |= ((uint32_t)((uint16_t)((int16_t)((a>>24) & 0xFFFF) - (int16_t)((b>>24) & 0xFFFF))>>1) & 0xFFFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __USUB16(uint32_t a, uint32_t b) {
  uint32_t r = ((uint16_t)(a & 0xFFFF) - (uint16_t)(b & 0xFFFF)) & 0xFFFF;
  r |= ((uint32_t)((uint16_t)((a>>8) & 0xFFFF) - (uint16_t)((b>>8) & 0xFFFF)) & 0xFFFF)<<8;
  r |= ((uint32_t)((uint16_t)((a>>16) & 0xFFFF) - (uint16_t)((b>>16) & 0xFFFF)) & 0xFFFF)<<16;
  r |= ((uint32_t)((uint16_t)((a>>24) & 0xFFFF) - (uint16_t)((b>>24) & 0xFFFF)) & 0xFFFF)<<24;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UQSUB16(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  for (uint16_t i = 2; i != 0; --i) {
    uint32_t t = (uint16_t)(a & 0xFFFF) - (uint16_t)(b & 0xFFFF);
    t = (t >= (1U<<16)) ? (1U<<16)-1 : t;
    r |= (uint32_t)(t & 0xFFFF) << ((2-i)*16);
    a >>= 16;
    b >>= 16;
  }
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UHSUB16(uint32_t a, uint32_t b) {
  uint32_t r = (((uint16_t)(a & 0xFFFF) - (uint16_t)(b & 0xFFFF))>>1) & 0xFFFF;
  r |= ((uint32_t)(((uint16_t)((a>>8) & 0xFFFF) - (uint16_t)((b>>8) & 0xFFFF))>>1) & 0xFFFF)<<8;
  r |= ((uint32_t)(((uint16_t)((a>>16) & 0xFFFF) - (uint16_t)((b>>16) & 0xFFFF))>>1) & 0xFFFF)<<16;
  r |= ((uint32_t)(((uint16_t)((a>>24) & 0xFFFF) - (uint16_t)((b>>24) & 0xFFFF))>>1) & 0xFFFF)<<24;
  return r;
}

#define sadd16  __SADD16
#define qadd16  __QADD16
#define shadd16 __SHADD16
#define uadd16  __UADD16
#define uqadd16 __UQADD16
#define uhadd16 __UHADD16
#define ssub16  __SSUB16
#define qsub16  __QSUB16
#define shsub16 __SHSUB16
#define usub16  __USUB16
#define uqsub16 __UQSUB16
#define uhsub16 __UHSUB16

// 32-bit Arithmetic ----------------------------------------------

static inline __attribute__((always_inline)) uint32_t __QADD(uint32_t a, uint32_t b) {
  return (int32_t)a + (int32_t)b;
}

static inline __attribute__((always_inline)) uint32_t __QSUB(uint32_t a, uint32_t b) {
  return (int32_t)a - (int32_t)b;
}

#define qadd    __QADD
#define qsub    __QSUB

// Combined Add / Sub Operations ----------------------------------------------

static inline __attribute__((always_inline)) uint32_t __SASX(uint32_t a, uint32_t b) {
  uint32_t r = ((int16_t)(a & 0xFFFF) - (int16_t)((b>>16) & 0xFFFF)) & 0xFFFF;
  r |= ((uint32_t)((int16_t)((a>>16) & 0xFFFF) + (int16_t)(b & 0xFFFF)) & 0xFFFF) << 16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __QASX(uint32_t a, uint32_t b) {
  int32_t t = (int16_t)(a & 0xFFFF) - (int16_t)((b>>16) & 0xFFFF);
  t = (t >= (1<<15)) ? (1<<15)-1 : (t <= -(1<<15)) ? -(1<<15) : t;
  uint32_t r = t;
  t = (int16_t)((a>>16) & 0xFFFF) + (int16_t)(b & 0xFFFF);
  t = (t >= (1<<15)) ? (1<<15)-1 : (t <= -(1<<15)) ? -(1<<15) : t;
  r |= t<<16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SHASX(uint32_t a, uint32_t b) {
  uint32_t r = (((int16_t)(a & 0xFFFF) - (int16_t)((b>>16) & 0xFFFF))>>1) & 0xFFFF;
  r |= ((uint32_t)(((int16_t)((a>>16) & 0xFFFF) + (int16_t)(b & 0xFFFF))>>1) & 0xFFFF) << 16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UASX(uint32_t a, uint32_t b) {
  uint32_t r = ((uint16_t)(a & 0xFFFF) - (uint16_t)((b>>16) & 0xFFFF)) & 0xFFFF;
  r |= ((uint32_t)((uint16_t)((a>>16) & 0xFFFF) + (uint16_t)(b & 0xFFFF)) & 0xFFFF) << 16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UQASX(uint32_t a, uint32_t b) {
  uint32_t t = (uint16_t)(a & 0xFFFF) - (uint16_t)((b>>16) & 0xFFFF);
  t = (t >= (1U<<16)) ? (1U<<16)-1 : t;
  uint32_t r = t;
  t = (uint16_t)((a>>16) & 0xFFFF) + (uint16_t)(b & 0xFFFF);
  t = (t >= (1U<<16)) ? (1U<<16)-1 : t;
  r |= t<<16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UHASX(uint32_t a, uint32_t b) {
  uint32_t r = (((uint16_t)(a & 0xFFFF) - (uint16_t)((b>>16) & 0xFFFF))>>1) & 0xFFFF;
  r |= ((uint32_t)(((uint16_t)((a>>16) & 0xFFFF) + (uint16_t)(b & 0xFFFF))>>1) & 0xFFFF) << 16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SSAX(uint32_t a, uint32_t b) {
  uint32_t r = ((int16_t)(a & 0xFFFF) + (int16_t)((b>>16) & 0xFFFF)) & 0xFFFF;
  r |= ((uint32_t)((int16_t)((a>>16) & 0xFFFF) - (int16_t)(b & 0xFFFF)) & 0xFFFF) << 16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __QSAX(uint32_t a, uint32_t b) {
  int32_t t = (int16_t)(a & 0xFFFF) + (int16_t)((b>>16) & 0xFFFF);
  t = (t >= (1<<15)) ? (1<<15)-1 : (t <= -(1<<15)) ? -(1<<15) : t;
  uint32_t r = t;
  t = (int16_t)((a>>16) & 0xFFFF) - (int16_t)(b & 0xFFFF);
  t = (t >= (1<<15)) ? (1<<15)-1 : (t <= -(1<<15)) ? -(1<<15) : t;
  r |= t<<16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SHSAX(uint32_t a, uint32_t b) {
  uint32_t r = (((int16_t)(a & 0xFFFF) + (int16_t)((b>>16) & 0xFFFF))>>1) & 0xFFFF;
  r |= ((uint32_t)(((int16_t)((a>>16) & 0xFFFF) - (int16_t)(b & 0xFFFF))>>1) & 0xFFFF) << 16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __USAX(uint32_t a, uint32_t b) {
  uint32_t r = ((uint16_t)(a & 0xFFFF) + (uint16_t)((b>>16) & 0xFFFF)) & 0xFFFF;
  r |= ((uint32_t)((uint16_t)((a>>16) & 0xFFFF) - (uint16_t)(b & 0xFFFF)) & 0xFFFF) << 16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UQSAX(uint32_t a, uint32_t b) {
  uint32_t t = (uint16_t)(a & 0xFFFF) + (uint16_t)((b>>16) & 0xFFFF);
  t = (t >= (1U<<16)) ? (1U<<16)-1 : t;
  uint32_t r = t;
  t = (uint16_t)((a>>16) & 0xFFFF) - (uint16_t)(b & 0xFFFF);
  t = (t >= (1U<<16)) ? (1U<<16)-1 : t;
  r |= t<<16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __UHSAX(uint32_t a, uint32_t b) {
  uint32_t r = (((uint16_t)(a & 0xFFFF) + (uint16_t)((b>>16) & 0xFFFF))>>1) & 0xFFFF;
  r |= ((uint32_t)(((uint16_t)((a>>16) & 0xFFFF) - (uint16_t)(b & 0xFFFF))>>1) & 0xFFFF) << 16;
  return r;
}

#define sasx    __SASX
#define qasx    __QASX
#define shasx   __SHASX
#define uasx    __UASX
#define uqasx   __UQASX
#define uhasx   __UHASX

#define ssax    __SSAX
#define qsax    __QSAX
#define shsax   __SHSAX
#define usax    __USAX
#define uqsax   __UQSAX
#define uhsax   __UHSAX

// Absolute Differences ----------------------------------------------

static inline __attribute__((always_inline)) uint32_t __USAD8(uint32_t a, uint32_t b) {
  uint32_t r = ((uint8_t)(a & 0xFF) - (uint8_t)(b & 0xFF)) & 0xFF;
  r += ((uint32_t)((uint8_t)((a>>8) & 0xFF) - (uint8_t)((b>>8) & 0xFF)) & 0xFF);
  r += ((uint32_t)((uint8_t)((a>>16) & 0xFF) - (uint8_t)((b>>16) & 0xFF)) & 0xFF);
  r += ((uint32_t)((uint8_t)((a>>24) & 0xFF) - (uint8_t)((b>>24) & 0xFF)) & 0xFF);
  return r;
}

static inline __attribute__((always_inline)) uint32_t __USADA8(uint32_t a, uint32_t b, uint32_t c) {
  uint32_t r = ((uint8_t)(a & 0xFF) - (uint8_t)(b & 0xFF)) & 0xFF;
  r += ((uint32_t)((uint8_t)((a>>8) & 0xFF) - (uint8_t)((b>>8) & 0xFF)) & 0xFF);
  r += ((uint32_t)((uint8_t)((a>>16) & 0xFF) - (uint8_t)((b>>16) & 0xFF)) & 0xFF);
  r += ((uint32_t)((uint8_t)((a>>24) & 0xFF) - (uint8_t)((b>>24) & 0xFF)) & 0xFF);
  return c+r;
}

#define usad8   __USAD8
#define usada8  __USADA8

// Saturation --------------------------------------------------------

static inline __attribute__((always_inline)) uint32_t __SSAT16(uint32_t v, uint32_t sat) {
  int32_t t = (int16_t)(v & 0xFFFF);
  const int32_t ref = 1U<<(sat-1);
  uint32_t r = (uint32_t)((t >= ref) ? ref-1 : (t <= -ref) ? -ref : t);
  t = (int16_t)(v>>16);
  r |= (uint32_t)((t >= ref) ? ref-1 : (t <= -ref) ? -ref : t)<<16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __USAT16(uint32_t v, uint32_t sat) {
  uint32_t t = (uint16_t)(v & 0xFFFF);
  const uint32_t ref = 1U<<sat;
  uint32_t r = (uint32_t)((t >= ref) ? ref-1 : t);
  t = (uint16_t)(v>>16);
  r |= (uint32_t)((t >= ref) ? ref-1 : t)<<16;
  return r;
}

#define ssat16  __SSAT16
#define usat16  __USAT16

// 8-bit -> 16-bit expansion ----------------------------------------

static inline __attribute__((always_inline)) uint32_t __UXTB16(uint32_t a) {
  return (a & 0xFF) | (((a>>16) & 0xFF)<<16);
}

static inline __attribute__((always_inline)) uint32_t __UXTAB16(uint32_t a, uint32_t b) {
  uint32_t r = ((uint16_t)(a & 0xFFFF) + (uint16_t)(b & 0xFF)) & 0xFFFF;
  r |= (uint32_t)(((uint16_t)(a>>16) + (uint16_t)((b>>16) & 0xFF)) & 0xFFFF) << 16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SXTB16(uint32_t a) {
  uint32_t r = (int16_t)(a & 0xFF) & 0xFFFF;
  r |= ((int16_t)((a>>16) & 0xFF) & 0xFFFF)<<16;
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SXTAB16(uint32_t a, uint32_t b) {
  uint32_t r = ((int16_t)(a & 0xFFFF) + (int16_t)(b & 0xFF)) & 0xFFFF;
  r |= (uint32_t)(((int16_t)(a>>16) + (int16_t)((b>>16) & 0xFF))) << 16;
  return r;
}

#define uxtb16  __UXTB16
#define uxtab16 __UXTAB16
#define sxtb16  __SXTB16
#define sxtab16 __SXTAB16

// Multiply Accumulate  ------------------------------------------

static inline __attribute__((always_inline)) uint32_t __SMUAD(uint32_t a, uint32_t b) {
  int32_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b & 0xFFFF);
  r += (int16_t)(a>>16) * (int16_t)(b>>16);
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SMUADX(uint32_t a, uint32_t b) {
  int32_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b>>16);
  r += (int16_t)(a>>16) * (int16_t)(b & 0xFFFF);
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SMLAD(uint32_t a, uint32_t b, uint32_t c) {
  int32_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b & 0xFFFF);
  r += (int16_t)(a>>16) * (int16_t)(b>>16);
  return r+c;
}

static inline __attribute__((always_inline)) uint32_t __SMLADX(uint32_t a, uint32_t b, uint32_t c) {
  int32_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b>>16);
  r += (int16_t)(a>>16) * (int16_t)(b & 0xFFFF);
  return r+c;
}

static inline __attribute__((always_inline)) uint64_t __SMLALD(uint32_t a, uint32_t b, uint64_t c) {
  int64_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b & 0xFFFF);
  r += (int16_t)(a>>16) * (int16_t)(b>>16);
  return r+c;
}

static inline __attribute__((always_inline)) uint64_t __SMLALDX(uint32_t a, uint32_t b, uint64_t c) {
  int64_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b>>16);
  r += (int16_t)(a>>16) * (int16_t)(b & 0xFFFF);
  return r+c;
}


static inline __attribute__((always_inline)) uint32_t __SMUSD(uint32_t a, uint32_t b) {
  int32_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b & 0xFFFF);
  r -= (int16_t)(a>>16) * (int16_t)(b>>16);
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SMUSDX(uint32_t a, uint32_t b) {
  int32_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b>>16);
  r -= (int16_t)(a>>16) * (int16_t)(b & 0xFFFF);
  return r;
}

static inline __attribute__((always_inline)) uint32_t __SMLSD(uint32_t a, uint32_t b, uint32_t c) {
  int32_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b & 0xFFFF);
  r -= (int16_t)(a>>16) * (int16_t)(b>>16);
  return r+c;
}

static inline __attribute__((always_inline)) uint32_t __SMLSDX(uint32_t a, uint32_t b, uint32_t c) {
  int32_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b>>16);
  r -= (int16_t)(a>>16) * (int16_t)(b & 0xFFFF);
  return r+c;
}

static inline __attribute__((always_inline)) uint64_t __SMLSLD(uint32_t a, uint32_t b, uint64_t c) {
  int64_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b & 0xFFFF);
  r -= (int16_t)(a>>16) * (int16_t)(b>>16);
  return r+c;
}

static inline __attribute__((always_inline)) uint64_t __SMLSLDX(uint32_t a, uint32_t b, uint64_t c) {
  int64_t r = (int16_t)(a & 0xFFFF) * (int16_t)(b>>16);
  r -= (int16_t)(a>>16) * (int16_t)(b & 0xFFFF);
  return r+c;
}

static inline __attribute__((always_inline)) uint32_t __SMMLA(uint32_t a, uint32_t b, uint32_t c) {
  int64_t r = (int32_t)a * (int32_t)b;
  return (r>>32)+c;
}


#define smuad   __SMUAD
#define smuadx  __SMUADX
#define smlad   __SMLAD
#define smladx  __SMLADX
#define smlald  __SMLALD
#define smlaldx __SMLALDX
#define smusd   __SMUSD
#define smusdx  __SMUSDX
#define smlsd   __SMLSD
#define smlsdx  __SMLSDX
#define smlsld  __SMLSLD
#define smlsldx __SMLSLDX
#define smmla   __SMMLA

// Other Operations ---------------------------------------------------

static inline __attribute__((always_inline)) void __SEL(uint32_t a, uint32_t b) { (void)a; (void)b; }

static inline __attribute__((always_inline)) uint32_t __PKHBT(uint32_t a, uint32_t b, uint32_t c) {
  return (((int32_t)a <<  0) & (int32_t)0x0000FFFF) | (((int32_t)b << c) & (int32_t)0xFFFF0000);
}

static inline __attribute__((always_inline)) uint32_t __PKHTB(uint32_t a, uint32_t b, uint32_t c) {
  return (((int32_t)a <<  0) & (int32_t)0xFFFF0000) | (((int32_t)b >> c) & (int32_t)0x0000FFFF);
}

#define sel     __SEL
#define pkhbt   __PKHBT
#define pkhtb   __PKHTB

#endif // __compat_h

/** @} */
