/**
 * @file    fixed_simd.h
 * @brief   SIMD Fixed Point Utilities.
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_fixed_simd Fixed-Point SIMD utilities
 * @{
 *
 */

#ifndef __fixed_simd_h
#define __fixed_simd_h

#include <math.h>
#include <stdint.h>

#if defined(__ARM_NEON__)
#include <arm_neon.h>
#define NEON_SIMD_FIXED 1
#endif

#include "int_simd.h"
#include "float_simd.h"

/*===========================================================================*/
/* Types.                                                                    */
/*===========================================================================*/

/**
 * @name    Types
 * @todo    Add more as needed...
 * @{
 */

typedef int32x2_t q23x2_t;

typedef int32x2_t q31x2_t;
typedef int32x2_t q1_30x2_t;
typedef int32x2_t q2_29x2_t;
typedef int32x2_t q3_28x2_t;
typedef int32x2_t q4_27x2_t;
typedef int32x2_t q5_26x2_t;
typedef int32x2_t q6_25x2_t;
typedef int32x2_t q7_24x2_t;
typedef int32x2_t q8_23x2_t;
typedef int32x2_t q15_16x2_t;
typedef int32x2_t q16_15x2_t;
typedef int32x2_t q17_14x2_t;
typedef int32x2_t q18_13x2_t;
typedef int32x2_t q19_12x2_t;
typedef int32x2_t q20_11x2_t;
typedef int32x2_t q21_10x2_t;
typedef int32x2_t q22_9x2_t;
typedef int32x2_t q23_8x2_t;
typedef int32x2_t q31_0x2_t;

typedef uint32x2_t uq32x2_t;
typedef uint32x2_t uq14_18x2_t;
typedef uint32x2_t uq32_0x2_t;

typedef int32x4_t q23x4_t;

typedef int32x4_t q31x4_t;
typedef int32x4_t q1_30x4_t;
typedef int32x4_t q2_29x4_t;
typedef int32x4_t q3_28x4_t;
typedef int32x4_t q4_27x4_t;
typedef int32x4_t q5_26x4_t;
typedef int32x4_t q6_25x4_t;
typedef int32x4_t q7_24x4_t;
typedef int32x4_t q8_23x4_t;
typedef int32x4_t q15_16x4_t;
typedef int32x4_t q16_15x4_t;
typedef int32x4_t q17_14x4_t;
typedef int32x4_t q18_13x4_t;
typedef int32x4_t q19_12x4_t;
typedef int32x4_t q20_11x4_t;
typedef int32x4_t q21_10x4_t;
typedef int32x4_t q22_9x4_t;
typedef int32x4_t q23_8x4_t;
typedef int32x4_t q31_0x4_t;

typedef uint32x4_t uq32x4_t;
typedef uint32x4_t uq14_18x4_t;
typedef uint32x4_t uq32_0x4_t;

/** @} */

/*===========================================================================*/
/* SIMD Operations.                                                          */
/*===========================================================================*/

/**
 * @name    SIMD Operations
 * @todo    Add more wrappers if needed. Full coverage would be quite tedious so if in depth
 *          optimization is required consider checking for NEON_SIMD_FIXED and use NEON intrinsics
 *          directly
 * @{
 */

// 32x2 Conversions to float -----------------------------------------------

static inline __attribute__((optimize("Ofast"), always_inline))
float32x2_t
q23x2_to_f32x2(const q23x2_t p) {
#if defined(NEON_SIMD_FIXED)
  // TODO: could likely get rid of the shift
  return vmul_n_f32(vcvt_f32_s32(vshl_n_s32(p, 8)), q31_to_f32_c);
  //return vcvt_n_f32_s32(p, 24);
#else
  const float32x2_t v = {{q31_to_f32(p.val[0] << 8), q31_to_f32(p.val[1] << 8)}};
  return v;
#endif
}

static inline __attribute__((optimize("Ofast"), always_inline))
float32x2_t
q31x2_to_f32x2(const q31x2_t p) {
#if defined(NEON_SIMD_FIXED)
  return vmul_n_f32(vcvt_f32_s32(p), q31_to_f32_c);
#else
  const float32x2_t v = {{q31_to_f32(p.val[0]), q31_to_f32(p.val[1])}};
  return v;
#endif
}

static inline __attribute__((optimize("Ofast"), always_inline))
float32x2_t
uq32x2_to_f32x2(const uq32x2_t p) {
#if defined(NEON_SIMD_FIXED)
  return vmul_n_f32(vcvt_f32_u32(p), uq32_to_f32_c);
#else
  const float32x2_t v = {{uq32_to_f32(p.val[0]), uq32_to_f32(p.val[1])}};
  return v;
#endif
}

// 32x2 Conversions from float -----------------------------------------------

static inline __attribute__((optimize("Ofast"), always_inline))
q23x2_t
f32x2_to_q23x2(const float32x2_t p) {
#if defined(NEON_SIMD_FIXED)
  // TODO: could likely get rid of the shift
  return vshr_n_s32(vcvt_s32_f32(vmul_n_f32(p, 0x7FFFFFFF)), 8);
  //return vcvt_n_s32_f32(p, 24);
#else
  const int32x2_t v = {{f32_to_q31(p.val[0]) >> 8, f32_to_q31(p.val[1]) >> 8}};
  return v;
#endif
}

static inline __attribute__((optimize("Ofast"), always_inline))
q31x2_t
f32x2_to_q31x2(const float32x2_t p) {
#if defined(NEON_SIMD_FIXED)
  return vcvt_s32_f32(vmul_n_f32(p, 0x7FFFFFFF));
#else
  const int32x2_t v = {{f32_to_q31(p.val[0]), f32_to_q31(p.val[1])}};
  return v;
#endif
}

static inline __attribute__((optimize("Ofast"), always_inline))
uq32x2_t
f32x2_to_uq32x2(const float32x2_t p) {
#if defined(NEON_SIMD_FIXED)
  return vcvt_u32_f32(vmul_n_f32(p, 0xFFFFFFFF));
#else
  const uint32x2_t v = {{f32_to_uq32(p.val[0]), f32_to_uq32(p.val[1])}};
  return v;
#endif
}

// 32x4 Conversions to float -----------------------------------------------

static inline __attribute__((optimize("Ofast"), always_inline))
float32x4_t
q23x4_to_f32x4(const q23x4_t p) {
#if defined(NEON_SIMD_FIXED)
  //TODO: could likely get rid of the shift
  return vmulq_n_f32(vcvtq_f32_s32(vshlq_n_s32(p, 8)), q31_to_f32_c);
  //return vcvtq_n_f32_s32(p, 24);
#else
  const float32x4_t v = {{q31_to_f32(p.val[0] << 8), q31_to_f32(p.val[1] << 8),
                          q31_to_f32(p.val[2] << 8), q31_to_f32(p.val[3] << 8)}};
  return v;
#endif
}

static inline __attribute__((optimize("Ofast"), always_inline))
float32x4_t
q31x4_to_f32x4(const q31x4_t p) {
#if defined(NEON_SIMD_FIXED)
  return vmulq_n_f32(vcvtq_f32_s32(p), q31_to_f32_c);
#else
  const float32x4_t v = {{q31_to_f32(p.val[0]), q31_to_f32(p.val[1]),
                          q31_to_f32(p.val[2]), q31_to_f32(p.val[3])}};
  return v;
#endif
}

static inline __attribute__((optimize("Ofast"), always_inline))
float32x4_t
uq32x4_to_f32x4(const uq32x4_t p) {
#if defined(NEON_SIMD_FIXED)
  return vmulq_n_f32(vcvtq_f32_u32(p), uq32_to_f32_c);
#else
  const float32x4_t v = {{uq32_to_f32(p.val[0]), uq32_to_f32(p.val[1]),
                          uq32_to_f32(p.val[2]), uq32_to_f32(p.val[3])}};
  return v;
#endif
}

// 32x4 Conversions from float -----------------------------------------------

static inline __attribute__((optimize("Ofast"), always_inline))
q23x4_t
f32x4_to_q23x4(const float32x4_t p) {
#if defined(NEON_SIMD_FIXED)
  // TODO: could likely get rid of the shift
  return vshrq_n_s32(vcvtq_s32_f32(vmulq_n_f32(p, 0x7FFFFFFF)), 8);
  //return vcvtq_n_s32_f32(p, 24);
#else
  const int32x4_t v = {{f32_to_q31(p.val[0]) >> 8, f32_to_q31(p.val[1]) >> 8,
                        f32_to_q31(p.val[2]) >> 8, f32_to_q31(p.val[3]) >> 8}};
  return v;
#endif
}

static inline __attribute__((optimize("Ofast"), always_inline))
q31x4_t
f32x4_to_q31x4(const float32x4_t p) {
#if defined(NEON_SIMD_FIXED)
  return vcvtq_s32_f32(vmulq_n_f32(p, 0x7FFFFFFF));
#else
  const int32x4_t v = {{f32_to_q31(p.val[0]), f32_to_q31(p.val[1]),
                        f32_to_q31(p.val[2]), f32_to_q31(p.val[3])}};
  return v;
#endif
}

static inline __attribute__((optimize("Ofast"), always_inline))
uq32x4_t
f32x4_to_uq32x4(const float32x4_t p) {
#if defined(NEON_SIMD_FIXED)
  return vcvtq_u32_f32(vmulq_n_f32(p, 0xFFFFFFFF));
#else
  const uint32x4_t v = {{f32_to_uq32(p.val[0]), f32_to_uq32(p.val[1]),
                         f32_to_uq32(p.val[2]), f32_to_uq32(p.val[3])}};
  return v;
#endif
}

/** @} */

#endif  // __fixed_simd_h
