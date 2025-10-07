#ifndef __fixed_math_h
#define __fixed_math_h

#include "utils/common_fixed_math.h"
#include "cortexa7_intrinsics.h"
#if defined(__arm__) && defined(__ARM_ACLE)
#include <arm_acle.h>
#endif

typedef int32_t simd32_t;

static inline __attribute__((optimize("Ofast"), always_inline))
int32_t
i32_ssat(int32_t a, uint32_t bits) 
{
  int32_t pos_max = 1;
  for (; bits > 0; --bits) 
  {
    pos_max *= 2;
  }

  if (a > 0) 
  {
    pos_max = (pos_max - 1);
    if (a > pos_max)
      a = pos_max;
  } 
  else 
  {
    const int32_t neg_min = -pos_max;
    if (a < neg_min)
      a = neg_min;
  }
  return a;
}

#define f32_to_q15_sat(f) ((q15_t)i32_ssat((q31_t)((float)(f) * ((1<<15)-1)),16))

#define q15add(a,b) ((q15_t)(qadd16((q15_t)(a),(q15_t)(b)) & 0xFFFF))
#define q15sub(a,b) ((q15_t)(qsub16((q15_t)(a),(q15_t)(b)) & 0xFFFF))
#define q15mul(a,b) ((q15_t)(((int32_t)(q15_t)(a) * (q15_t)(b))>>15))
#define q15absmul(a,b) (-q15mul(a, -b))
#define q15abs(a)   ((q15_t)(qsub16(((q15_t)(a) ^ ((q15_t)(a)>>15)), ((q15_t)(a)>>15)) & 0xFFFF))

/** Maximum
 */
static inline __attribute__((optimize("Ofast"),always_inline))
q15_t q15max(q15_t a, q15_t b) {
  qsub16(a,b);
  return sel(a,b);
}

/** Minimum
 */
static inline __attribute__((optimize("Ofast"),always_inline))
q15_t q15min(q15_t a, q15_t b) {
  qsub16(b,a);
  return sel(a,b);
}

#define q15addp(a,b) ((simd32_t)(qadd16((simd32_t)(a),(simd32_t)(b))))
#define q15subp(a,b) ((simd32_t)(qsub16((simd32_t)(a),(simd32_t)(b))))
#define q15absp(a)   ((simd32_t)(qsub16((simd32_t)(a) ^ ((simd32_t)(a)>>15), (simd32_t)(a)>>15)))

/** Maximum
 */
static inline __attribute__((optimize("Ofast"),always_inline))
simd32_t q15maxp(simd32_t a, simd32_t b) {
  qsub16(a,b);
  return sel(a,b);
}

/** Minimum
 */
static inline __attribute__((optimize("Ofast"),always_inline))
simd32_t q15minp(simd32_t a, simd32_t b) {
  qsub16(b,a);
  return sel(a,b);
}

/** @} */

/**
 * @name   Q31.
 * @note   Some arguments are used multiple times, make sure not to pass expressions.
 * @{
 */

#define q31add(a,b) (qadd((q31_t)(a),(q31_t)(b)))
#define q31sub(a,b) (qsub((q31_t)(a),(q31_t)(b)))
#define q31mul(a,b) ((q31_t)(((q63_t)(q31_t)(a) * (q31_t)(b))>>31))
#define q31absmul(a,b) (-q31mul(a,-b))
#define q31abs(a)   (qsub((q31_t)(a) ^ ((q31_t)(a)>>31), (q31_t)(a)>>31))

/** Maximum
 */
static inline __attribute__((optimize("Ofast"),always_inline))
q31_t q31max(q31_t a, q31_t b) {
  qsub(a,b);
  return sel(a,b);
}

/** Minimum
 */
static inline __attribute__((optimize("Ofast"),always_inline))
q31_t q31min(q31_t a, q31_t b) {
  qsub(b,a);
  return sel(a,b);
}

/** @} */

#include "fixed_simd.h"

#endif