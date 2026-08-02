/*
 * websim host shim for CMSIS <arm_math.h> (gen-1 / legacy logue SDK).
 *
 * The gen-1 oscillator runtime (prologue / minilogue xd / NTS-1 mkI) reaches
 * CMSIS Cortex-M4 intrinsics through inc/utils/cortexm4.h -> "arm_math.h", which
 * does not exist for the wasm target. This shim provides plain-C equivalents of
 * the handful of intrinsics the gen-1 fixed-point helpers actually use (see
 * WEBSIM_EXPANSION_PLAN.md §6.2). It is found ahead of the (absent) CMSIS header
 * via -I websim/dsp/legacy.
 *
 * Note: the q7_t/q15_t/q31_t/q63_t typedefs are intentionally NOT defined here —
 * inc/utils/fixed_math.h defines them itself right after including this shim.
 */

#ifndef WEBSIM_LEGACY_ARM_MATH_H_
#define WEBSIM_LEGACY_ARM_MATH_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Signed saturation to ARG2 bits: clamp to [-2^(n-1), 2^(n-1)-1]. */
static inline int32_t __websim_ssat(int32_t val, uint32_t bits)
{
  const int32_t max = (int32_t)((1u << (bits - 1)) - 1u);
  const int32_t min = -max - 1;
  return val > max ? max : (val < min ? min : val);
}

/* Unsigned saturation to ARG2 bits: clamp to [0, 2^n - 1]. */
static inline uint32_t __websim_usat(int32_t val, uint32_t bits)
{
  const int32_t max = (int32_t)((1u << bits) - 1u);
  return val < 0 ? 0 : (val > max ? (uint32_t)max : (uint32_t)val);
}

/* Saturating signed 32-bit add / subtract (Q31). */
static inline int32_t __websim_qadd(int32_t a, int32_t b)
{
  int64_t s = (int64_t)a + (int64_t)b;
  if (s > INT32_MAX) s = INT32_MAX;
  if (s < INT32_MIN) s = INT32_MIN;
  return (int32_t)s;
}

static inline int32_t __websim_qsub(int32_t a, int32_t b)
{
  int64_t s = (int64_t)a - (int64_t)b;
  if (s > INT32_MAX) s = INT32_MAX;
  if (s < INT32_MIN) s = INT32_MIN;
  return (int32_t)s;
}

/* Signed most-significant-word multiply-accumulate: acc + ((a*b) >> 32). */
static inline int32_t __websim_smmla(int32_t a, int32_t b, int32_t acc)
{
  return acc + (int32_t)(((int64_t)a * (int64_t)b) >> 32);
}

static inline uint32_t __websim_ror(uint32_t x, uint32_t n)
{
  n &= 31u;
  return n ? ((x >> n) | (x << (32u - n))) : x;
}

static inline uint32_t __websim_rbit(uint32_t x)
{
  uint32_t r = 0;
  for (int i = 0; i < 32; ++i) { r = (r << 1) | (x & 1u); x >>= 1; }
  return r;
}

/* Dual 16-bit saturating add / subtract (Cortex-M4 SIMD). */
static inline int16_t __websim_sat16(int32_t v)
{
  return (int16_t)(v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
}

static inline uint32_t __websim_qadd16(uint32_t a, uint32_t b)
{
  int16_t lo = __websim_sat16((int32_t)(int16_t)(a & 0xFFFF) + (int32_t)(int16_t)(b & 0xFFFF));
  int16_t hi = __websim_sat16((int32_t)(int16_t)(a >> 16) + (int32_t)(int16_t)(b >> 16));
  return ((uint32_t)(uint16_t)lo) | ((uint32_t)(uint16_t)hi << 16);
}

static inline uint32_t __websim_qsub16(uint32_t a, uint32_t b)
{
  int16_t lo = __websim_sat16((int32_t)(int16_t)(a & 0xFFFF) - (int32_t)(int16_t)(b & 0xFFFF));
  int16_t hi = __websim_sat16((int32_t)(int16_t)(a >> 16) - (int32_t)(int16_t)(b >> 16));
  return ((uint32_t)(uint16_t)lo) | ((uint32_t)(uint16_t)hi << 16);
}

/* Cortex-M4 SIMD register type. */
typedef int32_t __SIMD32_TYPE;

#define __SSAT(ARG1, ARG2)        __websim_ssat((int32_t)(ARG1), (uint32_t)(ARG2))
#define __USAT(ARG1, ARG2)        __websim_usat((int32_t)(ARG1), (uint32_t)(ARG2))
#define __QADD(ARG1, ARG2)        __websim_qadd((int32_t)(ARG1), (int32_t)(ARG2))
#define __QSUB(ARG1, ARG2)        __websim_qsub((int32_t)(ARG1), (int32_t)(ARG2))
#define __SMMLA(ARG1, ARG2, ARG3) __websim_smmla((int32_t)(ARG1), (int32_t)(ARG2), (int32_t)(ARG3))
#define __CLZ(ARG1)               ((ARG1) == 0u ? 32u : (uint32_t)__builtin_clz((uint32_t)(ARG1)))
#define __ROR(ARG1, ARG2)         __websim_ror((uint32_t)(ARG1), (uint32_t)(ARG2))
#define __REV(ARG1)               __builtin_bswap32((uint32_t)(ARG1))
#define __REV16(ARG1)             ((uint32_t)__builtin_bswap16((uint16_t)(ARG1)))
#define __RBIT(ARG1)              __websim_rbit((uint32_t)(ARG1))
#define __NOP()                   ((void)0)
#define __QADD16(ARG1, ARG2)      __websim_qadd16((uint32_t)(ARG1), (uint32_t)(ARG2))
#define __QSUB16(ARG1, ARG2)      __websim_qsub16((uint32_t)(ARG1), (uint32_t)(ARG2))

// __SEL selects bytes using the GE flags set by a preceding SIMD op — a side
// effect that cannot be reproduced standalone on wasm. It is only reached from
// the q15/q31 max/min helpers, which the oscillator DSP path does not use, so a
// compile-only stub (returns the first operand) is sufficient for the sim.
#define __SEL(ARG1, ARG2)         (ARG1)

#ifdef __cplusplus
}
#endif

#endif // WEBSIM_LEGACY_ARM_MATH_H_
