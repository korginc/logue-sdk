/**
 * @file    mk2_float_simd.h
 * @brief   SIMD Floating Point Utilities.
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_float_simd Floating-Point SIMD utilities
 * @{
 *
 */

 #ifndef __mk2_float_simd_h
 #define __mk2_float_simd_h
 
 #include <math.h>
 #include <stdint.h>
 
 #if defined(__ARM_NEON) && defined(__ARM_FP)
 #include <arm_neon.h>
 #define NEON_SIMD_FP 1
 #endif
 
 #include "int_simd.h"
 
 /*===========================================================================*/
 /* Types.                                                                    */
 /*===========================================================================*/
 
 /**
  * @name    Types
  * @{
  */
 
 #if !defined(NEON_SIMD_FP)
 /* typedef float float32x2_t[2] __attribute__((aligned(4))); */
 typedef struct float32x2 {
   float val[2];
 } float32x2_t __attribute__((aligned(4)));
 #endif
 
 /** Make a float pair.
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2(const float a, const float b) {
 #if defined(NEON_SIMD_FP)
   const float32x2_t v = {a, b};
   return v;
 #else
   const float32x2_t v = {{a, b}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_FP)
 /* typedef float float32x4_t[4] __attribute__((aligned(4))); */
 typedef struct float32x4 {
   float val[4];
 } float32x4_t __attribute__((aligned(4)));
 #endif
 
 /** Make a float quad.
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4(const float a, const float b, const float c, const float d) {
 #if defined(NEON_SIMD_FP)
   const float32x4_t v = {a, b, c, d};
   return v;
 #else
   const float32x4_t v = {{a, b, c, d}};
   return v;
 #endif
 }
 
 #if defined(NEON_SIMD_FP)
 #define f32x2_lane(f, i) ((f)[(i)])
 #define f32x4_lane(f, i) ((f)[(i)])
 #else
 #define f32x2_lane(f, i) ((f).val[(i)])
 #define f32x4_lane(f, i) ((f).val[(i)])
 #endif
 
 #if defined(NEON_SIMD_FP)
 #define f32x2_const(c) \
   { (c), (c) }
 #define f32x4_const(c) \
   { (c), (c), (c), (c) }
 #else
 #define f32x2_const(c) \
   {                    \
     { (c), (c) }       \
   }
 #define f32x4_const(c)     \
   {                        \
     { (c), (c), (c), (c) } \
   }
 #endif
 
 #if !defined(NEON_SIMD_FP)
 typedef struct {
   float32x2_t val[2];
 } float32x2x2_t __attribute__((aligned(4)));
 #endif
 
 /** Make a float pair.
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2x2_t
 float32x2x2(const float32x2_t a, const float32x2_t b) {
   const float32x2x2_t v = {{a, b}};
   return v;
 }
 
 #if !defined(NEON_SIMD_FP)
 typedef struct {
   float32x4_t val[2];
 } float32x4x2_t __attribute__((aligned(4)));
 #endif
 
 /** Make a float quad.
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4x2_t
 float32x4x2(const float32x4_t a, const float32x4_t b) {
   const float32x4x2_t v = {{a, b}};
   return v;
 }
 
 typedef union {
   float32x2_t f;
   uint32x2_t u;
 } uf32x2_t;
 
 typedef union {
   float32x4_t f;
   uint32x4_t u;
 } uf32x4_t;
 
 /** @} */
 
 /*===========================================================================*/
 /* SIMD Operations.                                                          */
 /*===========================================================================*/
 
 /**
  * @name    SIMD Operations
  * @todo    Add more wrappers if needed. Full coverage would be quite tedious so if in depth
  *          optimization is required consider checking for NEON_SIMD_FP and use NEON intrinsics
  *          directly
  * @{
  */
 
 #if defined(NEON_SIMD_FP)
 #define f32x2_ld(ptr) vld1_f32((ptr))
 #define f32x4_ld(ptr) vld1q_f32((ptr))
 /* #define f32x2x2_ld(ptr) vld1_f32_x2((ptr)) */  // Missing from GCC
 // Note: must make sure to pass float32x2x2 as v
 #define f32x2x2_ld(ptr, v) asm volatile("\tvld1.32 %h0, [%1]\n" \
                                         : "=w"((v))             \
                                         : "r"((ptr))            \
                                         :)
 /* #define f32x4x2_ld(ptr) vld1q_f32_x2((ptr)) */  // Missing from GCC
 // Note: must make sure to pass float32x4x2 as v
 #define f32x4x2_ld(ptr, v) asm volatile("\tvld1.32 %h0, [%1]\n" \
                                         : "=w"((v))             \
                                         : "r"((ptr))            \
                                         :)
 #define f32x2x2_ld2(ptr) vld2_f32((ptr))   // interleaving type 2
 #define f32x4x2_ld2(ptr) vld2q_f32((ptr))  // interleaving type 2
 #define f32x2x2_ld2_dup(ptr) vld2_dup_f32((ptr))
 #define f32x4x2_ld2_dup(ptr) vld2q_dup_f32((ptr))
 #define f32x2_str(ptr, v) vst1_f32((ptr), (v))
 #define f32x4_str(ptr, v) vst1q_f32((ptr), (v))
 /* #define f32x2x2_str(ptr, v) vst1_f32_x2((ptr), (v)) */  // Missing from GCC
 #define f32x2x2_str(ptr, v) asm volatile("\tvst1.32 %h0, [%1]!\n"              \
                                          :                                     \
                                          : "w"((float32x2x2_t)(v)), "r"((ptr)) \
                                          :)
 /* #define f32x4x2_str(ptr, v) vst1q_f32_x2((ptr), (v)) */  // Missing from GCC
 #define f32x4x2_str(ptr, v) asm volatile("\tvst1.32 %h0, [%1]!\n"              \
                                          :                                     \
                                          : "w"((float32x4x2_t)(v)), "r"((ptr)) \
                                          :)
 #define f32x2x2_str2(ptr, v) vst2_f32((ptr), (v))   // interleaving type 2
 #define f32x4x2_str2(ptr, v) vst2q_f32((ptr), (v))  // interleaving type 2
 #define f32x2_dup(c) vdup_n_f32((c))
 #define f32x4_dup(c) vdupq_n_f32((c))
 // TODO: add more dup variants, with inline asm if needed
 #else
 #define f32x2_ld(ptr) (*(const float32x2_t *)(ptr))
 #define f32x4_ld(ptr) (*(const float32x4_t *)(ptr))
 #define f32x2x2_ld(ptr) (*(const float32x2x2_t *)(ptr))
 #define f32x4x2_ld(ptr) (*(const float32x4x2_t *)(ptr))
 // TODO: add f32x2x2_ld2 and f32x4x2_ld2
 // TODO: add f32x2x2_ld2_dup and f32x4x2_ld2_dup
 #define f32x2_str(ptr, v)        \
   do {                           \
     *(float32x2_t *)(ptr) = (v); \
   } while (0);                   
 #define f32x4_str(ptr, v)        \
   do {                           \
     *(float32x4_t *)(ptr) = (v); \
   } while (0);
 #define f32x2x2_str(ptr, v)       \
   do {                            \
     *(float32x2x2_t *)(ptr) = (v); \
   } while (0);
 #define f32x4x2_str(ptr, v)       \
   do {                            \
     *(float32x4x2_t *)(ptr) = (v); \
   } while (0);
 #define f32x2x2_str2(ptr, v)            \
   do {                                  \
     (ptr)[0] = (float32x4_t)(v).val[0]; \
     (ptr)[1] = (float32x4_t)(v).val[2]; \
     (ptr)[2] = (float32x4_t)(v).val[1]; \
     (ptr)[3] = (float32x4_t)(v).val[3]; \
   } while (0);
 #define f32x4x2_str2(ptr, v)                     \
   do {                                           \
     (ptr)[0] = (float32x4x2_t)(v).val[0].val[0]; \
     (ptr)[1] = (float32x4x2_t)(v).val[1].val[0]; \
     (ptr)[2] = (float32x4x2_t)(v).val[0].val[1]; \
     (ptr)[3] = (float32x4x2_t)(v).val[1].val[1]; \
     (ptr)[4] = (float32x4x2_t)(v).val[0].val[2]; \
     (ptr)[5] = (float32x4x2_t)(v).val[1].val[2]; \
     (ptr)[6] = (float32x4x2_t)(v).val[0].val[3]; \
     (ptr)[7] = (float32x4x2_t)(v).val[1].val[3]; \
   } while (0);
 #define f32x2_dup(c) float32x2((c), (c))
 #define f32x4_dup(c) float32x4((c), (c), (c), (c))
 #endif
 
 /** Addition
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_add(const float32x2_t p0, const float32x2_t p1) {
 #if defined(NEON_SIMD_FP)
   return vadd_f32(p0, p1);
 #else
   const float32x2_t v = {{p0.val[0] + p1.val[0], p0.val[1] + p1.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_add(const float32x4_t p0, const float32x4_t p1) {
 #if defined(NEON_SIMD_FP)
   return vaddq_f32(p0, p1);
 #else
   const float32x4_t v = {{p0.val[0] + p1.val[0], p0.val[1] + p1.val[1], p0.val[2] + p1.val[2], p0.val[3] + p1.val[3]}};
   return v;
 #endif
 }
 
 /** Pairwise Addition
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_padd(const float32x2_t p0, const float32x2_t p1) {
 #if defined(NEON_SIMD_FP)
   return vpadd_f32(p0, p1);
 #else
   const float32x2_t v = {{p0.val[0] + p0.val[1], p1.val[0] + p1.val[1]}};
   return v;
 #endif
 }
 
 // Note (Etienne): only available on ARM 64bit
 /* static inline __attribute__((optimize("Ofast"), always_inline)) */
 /* float32x4_t */
 /* float32x4_padd(const float32x4_t p0, const float32x4_t p1) { */
 /* #if defined(NEON_SIMD_FP) */
 /*   return vpaddq_f32(p0, p1); */
 /* #else */
 /*   const float32x4_t v = {{p0.val[0] + p0.val[1], p0.val[2] + p0.val[3], p1.val[0] + p1.val[1], p1.val[2] + p1.val[3]}}; */
 /*   return v; */
 /* #endif */
 /* } */
 
 /** subtraction
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_sub(const float32x2_t p0, const float32x2_t p1) {
 #if defined(NEON_SIMD_FP)
   return vsub_f32(p0, p1);
 #else
   const float32x2_t v = {{p0.val[0] - p1.val[0], p0.val[1] - p1.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_sub(const float32x4_t p0, const float32x4_t p1) {
 #if defined(NEON_SIMD_FP)
   return vsubq_f32(p0, p1);
 #else
   const float32x4_t v = {{p0.val[0] - p1.val[0], p0.val[1] - p1.val[1], p0.val[2] - p1.val[2], p0.val[3] - p1.val[3]}};
   return v;
 #endif
 }
 
 /** negate
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_neg(const float32x2_t p) {
 #if defined(NEON_SIMD_FP)
   return vneg_f32(p);
 #else
   const float32x2_t v = {{-p.val[0], -p.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_neg(const float32x4_t p) {
 #if defined(NEON_SIMD_FP)
   return vnegq_f32(p);
 #else
   const float32x4_t v = {{-p.val[0], -p.val[1], -p.val[2], -p.val[3]}};
   return v;
 #endif
 }
 
 /** scalar addition
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_addscal(const float32x2_t p, const float scl) {
 #if defined(NEON_SIMD_FP)
   return vadd_f32(p, vdup_n_f32(scl));
 #else
   const float32x2_t v = {{p.val[0] + scl, p.val[1] + scl}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_addscal(const float32x4_t p, const float scl) {
 #if defined(NEON_SIMD_FP)
   return vaddq_f32(p, vdupq_n_f32(scl));
 #else
   const float32x4_t v = {{p.val[0] + scl, p.val[1] + scl, p.val[2] + scl, p.val[3] + scl}};
   return v;
 #endif
 }
 
 /** scalar subtraction
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_subscal(const float32x2_t p, const float scl) {
 #if defined(NEON_SIMD_FP)
   return vsub_f32(p, vdup_n_f32(scl));
 #else
   const float32x2_t v = {{p.val[0] - scl, p.val[1] - scl}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_subscal(const float32x4_t p, const float scl) {
 #if defined(NEON_SIMD_FP)
   return vsubq_f32(p, vdupq_n_f32(scl));
 #else
   const float32x4_t v = {{p.val[0] - scl, p.val[1] - scl, p.val[2] - scl, p.val[3] - scl}};
   return v;
 #endif
 }
 
 /** multiply
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_mul(const float32x2_t p0, const float32x2_t p1) {
 #if defined(NEON_SIMD_FP)
   return vmul_f32(p0, p1);
 #else
   const float32x2_t v = {{p0.val[0] * p1.val[0], p0.val[1] * p1.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_mul(const float32x4_t p0, const float32x4_t p1) {
 #if defined(NEON_SIMD_FP)
   return vmulq_f32(p0, p1);
 #else
   const float32x4_t v = {{p0.val[0] * p1.val[0], p0.val[1] * p1.val[1], p0.val[2] * p1.val[2], p0.val[3] * p1.val[3]}};
   return v;
 #endif
 }
 
 /** multiply accumulate
  * Note: Prefer fused versions lower
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_mulacc(const float32x2_t acc, const float32x2_t p0, const float32x2_t p1) {
 #if defined(NEON_SIMD_FP)
   return vmla_f32(acc, p0, p1);
 #else
   const float32x2_t v = {{acc.val[0] + p0.val[0] * p1.val[0], acc.val[1] + p0.val[1] * p1.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_mulacc(const float32x4_t acc, const float32x4_t p0, const float32x4_t p1) {
 #if defined(NEON_SIMD_FP)
   return vmlaq_f32(acc, p0, p1);
 #else
   const float32x4_t v = {{acc.val[0] + p0.val[0] * p1.val[0], acc.val[1] + p0.val[1] * p1.val[1],
                           acc.val[2] + p0.val[2] * p1.val[2], acc.val[3] + p0.val[3] * p1.val[3]}};
   return v;
 #endif
 }
 
 /** multiply subtract
  * Note: Prefer fused versions lower
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_mulsub(const float32x2_t acc, const float32x2_t p0, const float32x2_t p1) {
 #if defined(NEON_SIMD_FP)
   return vmls_f32(acc, p0, p1);
 #else
   const float32x2_t v = {{acc.val[0] - p0.val[0] * p1.val[0], acc.val[1] - p0.val[1] * p1.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_mulsub(const float32x4_t acc, const float32x4_t p0, const float32x4_t p1) {
 #if defined(NEON_SIMD_FP)
   return vmlsq_f32(acc, p0, p1);
 #else
   const float32x4_t v = {{acc.val[0] - p0.val[0] * p1.val[0], acc.val[1] - p0.val[1] * p1.val[1],
                           acc.val[2] - p0.val[2] * p1.val[2], acc.val[3] - p0.val[3] * p1.val[3]}};
   return v;
 #endif
 }
 
 /** Fused multiply add
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_fmuladd(float32x2_t acc, const float32x2_t a, const float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vfma_f32(acc, a, b);
 #else
   const float32x2_t v = {{acc.val[0] + a.val[0] * b.val[0],
                           acc.val[1] + a.val[1] * b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_fmuladd(float32x4_t acc, const float32x4_t a, const float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vfmaq_f32(acc, a, b);
 #else
   const float32x4_t v = {{acc.val[0] + a.val[0] * b.val[0],
                           acc.val[1] + a.val[1] * b.val[1],
                           acc.val[2] + a.val[2] * b.val[2],
                           acc.val[3] + a.val[3] * b.val[3]}};
   return v;
 #endif
 }
 
 /** Fused multiply subtract
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_fmulsub(float32x2_t acc, const float32x2_t a, const float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vfms_f32(acc, a, b);
 #else
   const float32x2_t v = {{acc.val[0] - a.val[0] * b.val[0],
                           acc.val[1] - a.val[1] * b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_fmulsub(float32x4_t acc, const float32x4_t a, const float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vfmsq_f32(acc, a, b);
 #else
   const float32x4_t v = {{acc.val[0] - a.val[0] * b.val[0],
                           acc.val[1] - a.val[1] * b.val[1],
                           acc.val[2] - a.val[2] * b.val[2],
                           acc.val[3] - a.val[3] * b.val[3]}};
   return v;
 #endif
 }
 
 /** Fused multiply-scalar add
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_fmulscaladd(float32x2_t acc, const float32x2_t a, const float b) {
 #if defined(NEON_SIMD_FP)
   //return vfma_n_f32(acc, a, b); // Note: not supported by our GCC version?
   return vfma_f32(acc, a, vdup_n_f32(b));
 #else
   const float32x2_t v = {{acc.val[0] + a.val[0] * b,
                           acc.val[1] + a.val[1] * b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_fmulscaladd(float32x4_t acc, const float32x4_t a, const float b) {
 #if defined(NEON_SIMD_FP)
   //return vfmaq_n_f32(acc, a, b); // Note: not supported by our GCC version?
   return vfmaq_f32(acc, a, vdupq_n_f32(b));
 #else
   const float32x4_t v = {{acc.val[0] + a.val[0] * b,
                           acc.val[1] + a.val[1] * b,
                           acc.val[2] + a.val[2] * b,
                           acc.val[3] + a.val[3] * b}};
   return v;
 #endif
 }
 
 /** Fused multiply-scalar subtract
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_fmulscalsub(float32x2_t acc, const float32x2_t a, const float b) {
 #if defined(NEON_SIMD_FP)
   //return vfms_n_f32(acc, a, b); // Note: not supported by our GCC version?
   return vfms_f32(acc, a, vdup_n_f32(b));
 #else
   const float32x2_t v = {{acc.val[0] - a.val[0] * b,
                           acc.val[1] - a.val[1] * b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_fmulscalsub(float32x4_t acc, const float32x4_t a, const float b) {
 #if defined(NEON_SIMD_FP)
   //return vfmsq_n_f32(acc, a, b); // Note: not supported by our GCC version?
   return vfmsq_f32(acc, a, vdupq_n_f32(b));
 #else
   const float32x4_t v = {{acc.val[0] - a.val[0] * b,
                           acc.val[1] - a.val[1] * b,
                           acc.val[2] - a.val[2] * b,
                           acc.val[3] - a.val[3] * b}};
   return v;
 #endif
 }
 
 /** scalar multiply
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_mulscal(const float32x2_t p, const float scl) {
 #if defined(NEON_SIMD_FP)
   return vmul_n_f32(p, scl);
 #else
   const float32x2_t v = {{p.val[0] * scl, p.val[1] * scl}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_mulscal(const float32x4_t p, const float scl) {
 #if defined(NEON_SIMD_FP)
   return vmulq_n_f32(p, scl);
 #else
   const float32x4_t v = {{p.val[0] * scl, p.val[1] * scl, p.val[2] * scl, p.val[3] * scl}};
   return v;
 #endif
 }
 
 /** Reciprocal
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_rcp(const float32x2_t p) {
 #if defined(NEON_SIMD_FP)
   return vrecpe_f32(p);
 #else
   const float32x2_t v = {{1.f / p.val[0], 1.f / p.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_rcp(const float32x4_t p) {
 #if defined(NEON_SIMD_FP)
   return vrecpeq_f32(p);
 #else
   const float32x4_t v = {{1.f / p.val[0], 1.f / p.val[1], 1.f / p.val[2], 1.f / p.val[3]}};
   return v;
 #endif
 }
 
 /** Division
  */
 // Note (Etienne) : Only available on ARM 64bit, use recip + mult
 /* static inline __attribute__((optimize("Ofast"), always_inline)) */
 /* float32x2_t */
 /* float32x2_div(const float32x2_t p0, const float32x2_t p1) { */
 /* #if defined(NEON_SIMD_FP) */
 /*   return vdiv_f32(p0, p1); */
 /* #else */
 /*   const float32x2_t v = {{p0.val[0] / p1.val[0], p0.val[1] / p1.val[1]}}; */
 /*   return v; */
 /* #endif */
 /* } */
 
 /* static inline __attribute__((optimize("Ofast"), always_inline)) */
 /* float32x4_t */
 /* float32x4_div(const float32x4_t p0, const float32x4_t p1) { */
 /* #if defined(NEON_SIMD_FP) */
 /*   return vdivq_f32(p0, p1); */
 /* #else */
 /*   const float32x4_t v = {{p0.val[0] / p1.val[0], p0.val[1] / p1.val[1], p0.val[2] / p1.val[2], p0.val[3] / p1.val[3]}}; */
 /*   return v; */
 /* #endif */
 /* } */
 
 /** Split
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x4_low(const float32x4_t p) {
 #if defined(NEON_SIMD_FP)
   return vget_low_f32(p);
 #else
   const float32x2_t v = {{p.val[0], p.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x4_high(const float32x4_t p) {
 #if defined(NEON_SIMD_FP)
   return vget_high_f32(p);
 #else
   const float32x2_t v = {{p.val[2], p.val[3]}};
   return v;
 #endif
 }
 
 /** Combine
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x2_comb(const float32x2_t p0, const float32x2_t p1) {
 #if defined(NEON_SIMD_FP)
   return vcombine_f32(p0, p1);
 #else
   const float32x4_t v = {{p0.val[0], p0.val[1], p1.val[0], p1.val[1]}};
   return v;
 #endif
 }
 
 /** Minimum
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_min(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vmin_f32(a, b);
 #else
   const float32x2_t v = {{(a.val[0] < b.val[0]) ? a.val[0] : b.val[0],
                           (a.val[1] < b.val[1]) ? a.val[1] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_min(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vminq_f32(a, b);
 #else
   const float32x4_t v = {{(a.val[0] < b.val[0]) ? a.val[0] : b.val[0],
                           (a.val[1] < b.val[1]) ? a.val[1] : b.val[1],
                           (a.val[2] < b.val[2]) ? a.val[2] : b.val[2],
                           (a.val[3] < b.val[3]) ? a.val[3] : b.val[3]}};
   return v;
 #endif
 }
 
 /** Maximum
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_max(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vmax_f32(a, b);
 #else
   const float32x2_t v = {{(a.val[0] > b.val[0]) ? a.val[0] : b.val[0],
                           (a.val[1] > b.val[1]) ? a.val[1] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_max(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vmaxq_f32(a, b);
 #else
   const float32x4_t v = {{(a.val[0] > b.val[0]) ? a.val[0] : b.val[0],
                           (a.val[1] > b.val[1]) ? a.val[1] : b.val[1],
                           (a.val[2] > b.val[2]) ? a.val[2] : b.val[2],
                           (a.val[3] > b.val[3]) ? a.val[3] : b.val[3]}};
   return v;
 #endif
 }
 
 /** Pairwise Minimum
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_pmin(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vpmin_f32(a, b);
 #else
   // Note: unclear if a and b get inverted in the process, confirm.
   const float32x2_t v = {{(a.val[0] < a.val[1]) ? a.val[0] : a.val[1],
                           (b.val[0] < b.val[1]) ? b.val[0] : b.val[1]}};
   return v;
 #endif
 }
 
 /** Pairwise Maximum
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_pmax(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vpmax_f32(a, b);
 #else
   // Note: unclear if a and b get inverted in the process, confirm.
   const float32x2_t v = {{(a.val[0] > a.val[1]) ? a.val[0] : a.val[1],
                           (b.val[0] > b.val[1]) ? b.val[0] : b.val[1]}};
   return v;
 #endif
 }
 
 /** Less than
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_lt(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vclt_f32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] < b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] < b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_lt(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vcltq_f32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] < b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] < b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] < b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] < b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Less than or equal
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_lte(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vcle_f32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] <= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] <= b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_lte(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vcleq_f32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] <= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] <= b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] <= b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] <= b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Less than zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_ltz(float32x2_t a) {
 #if defined(NEON_SIMD_FP)
   //return vcltz_f32(a); // A64 only
   return vclt_f32(a, vdup_n_f32(0.f));
 #else
   const uint32x2_t v = {{(a.val[0] < 0.f) ? 0xFFFFFFFF : 0x0, (a.val[1] < 0.f) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_ltz(float32x4_t a) {
 #if defined(NEON_SIMD_FP)
   //return vcltzq_f32(a); // A64 only
   return vcltq_f32(a, vdupq_n_f32(0.f));
 #else
   const uint32x4_t v = {{(a.val[0] < 0.f) ? 0xFFFFFFFF : 0x0, (a.val[1] < 0.f) ? 0xFFFFFFFF : 0x0, (a.val[2] < 0.f) ? 0xFFFFFFFF : 0x0, (a.val[3] < 0.f) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Less than or equal zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_ltez(float32x2_t a) {
 #if defined(NEON_SIMD_FP)
   //return vclez_f32(a); // A64 only
   return vcle_f32(a, vdup_n_f32(0.f));
 #else
   const uint32x2_t v = {{(a.val[0] <= 0.f) ? 0xFFFFFFFF : 0x0, (a.val[1] <= 0.f) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_ltez(float32x4_t a) {
 #if defined(NEON_SIMD_FP)
   //return vclezq_f32(a); // A64 only
   return vcleq_f32(a, vdupq_n_f32(0.f));
 #else
   const uint32x4_t v = {{(a.val[0] <= 0.f) ? 0xFFFFFFFF : 0x0, (a.val[1] <= 0.f) ? 0xFFFFFFFF : 0x0, (a.val[2] <= 0.f) ? 0xFFFFFFFF : 0x0, (a.val[3] <= 0.f) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Greater than
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_gt(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vcgt_f32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] > b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] > b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_gt(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vcgtq_f32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] > b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] > b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] > b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] > b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Greater than or equal
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_gte(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vcge_f32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] >= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] >= b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_gte(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vcgeq_f32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] >= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] >= b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] >= b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] >= b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Greater than zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_gtz(float32x2_t a) {
 #if defined(NEON_SIMD_FP)
   //return vcgtz_f32(a); // A64 only
   return vcgt_f32(a, vdup_n_f32(0.f));
 #else
   const uint32x2_t v = {{(a.val[0] > 0.f) ? 0xFFFFFFFF : 0x0, (a.val[1] > 0.f) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_gtz(float32x4_t a) {
 #if defined(NEON_SIMD_FP)
   //return vcgtzq_f32(a); // A64 only
   return vcgtq_f32(a, vdupq_n_f32(0.f));
 #else
   const uint32x4_t v = {{(a.val[0] > 0.f) ? 0xFFFFFFFF : 0x0, (a.val[1] > 0.f) ? 0xFFFFFFFF : 0x0, (a.val[2] > 0.f) ? 0xFFFFFFFF : 0x0, (a.val[3] > 0.f) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Greater than or equal zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_gtez(float32x2_t a) {
 #if defined(NEON_SIMD_FP)
   //return vcgez_f32(a); // A64 only
   return vcge_f32(a, vdup_n_f32(0.f));
 #else
   const uint32x2_t v = {{(a.val[0] >= 0.f) ? 0xFFFFFFFF : 0x0, (a.val[1] >= 0.f) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_gtez(float32x4_t a) {
 #if defined(NEON_SIMD_FP)
   //return vcgezq_f32(a); // A64 only
   return vcgeq_f32(a, vdupq_n_f32(0.f));
 #else
   const uint32x4_t v = {{(a.val[0] >= 0.f) ? 0xFFFFFFFF : 0x0, (a.val[1] >= 0.f) ? 0xFFFFFFFF : 0x0, (a.val[2] >= 0.f) ? 0xFFFFFFFF : 0x0, (a.val[3] >= 0.f) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Equal
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 float32x2_eq(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vceq_f32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] == b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] == b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 float32x4_eq(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vceqq_f32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] == b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] == b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] == b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] == b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Select
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_sel(uint32x2_t s, float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vbsl_f32(s, a, b);
 #else
   // Note: note completely accurate since performing select word-wise instead of bit-wise but reflects typical usage
   const float32x2_t v = {{(s.val[0] != 0) ? a.val[0] : b.val[0], (s.val[1] != 0) ? a.val[1] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_sel(uint32x4_t s, float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vbslq_f32(s, a, b);
 #else
   // Note: note completely accurate since performing select word-wise instead of bit-wise but reflects typical usage
   const float32x4_t v = {{(s.val[0] != 0) ? a.val[0] : b.val[0], (s.val[1] != 0) ? a.val[1] : b.val[1], (s.val[2] != 0) ? a.val[2] : b.val[2], (s.val[3] != 0) ? a.val[3] : b.val[3]}};
   return v;
 #endif
 }
 
 /** Reverse vector order
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_rev(float32x2_t a) {
 #if defined(NEON_SIMD_FP)
   return vrev64_f32(a);
 #else
   const float32x2_t v = {{a.val[1], a.val[0]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_rev(float32x4_t a) {
 #if defined(NEON_SIMD_FP)
   return vrev64q_f32(a);
 #else
   const float32x4_t v = {{a.val[3], a.val[2], a.val[1], a.val[0]}};
   return v;
 #endif
 }
 
 /** Transpose vectors
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2x2_t
 float32x2_trn(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vtrn_f32(a, b);
 #else
   const float32x2x2_t v;
   v.val[0] = {{a.val[0], b.val[0]}};
   v.val[1] = {{a.val[1], b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4x2_t
 float32x4_trn(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vtrnq_f32(a, b);
 #else
   const float32x4x2_t v;
   v.val[0] = {{a.val[0], b.val[0], a.val[2], b.val[2]}};
   v.val[1] = {{a.val[1], b.val[1], a.val[3], b.val[3]}};
   return v;
 #endif
 }
 
 /** Zip vectors
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2x2_t
 float32x2_zip(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vzip_f32(a, b);
 #else
   const float32x2x2_t v;
   v.val[0] = {{a.val[0], b.val[0]}};
   v.val[1] = {{a.val[1], b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4x2_t
 float32x4_zip(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vzipq_f32(a, b);
 #else
   const float32x4x2_t v;
   v.val[0] = {{a.val[0], b.val[0], a.val[1], b.val[1]}};
   v.val[1] = {{a.val[2], b.val[2], a.val[3], b.val[3]}};
   return v;
 #endif
 }
 
 /** Unzip vectors
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2x2_t
 float32x2_unzip(float32x2_t a, float32x2_t b) {
 #if defined(NEON_SIMD_FP)
   return vuzp_f32(a, b);
 #else
   const float32x2x2_t v;
   v.val[0] = {{a.val[0], b.val[0]}};
   v.val[1] = {{a.val[1], b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4x2_t
 float32x4_unzip(float32x4_t a, float32x4_t b) {
 #if defined(NEON_SIMD_FP)
   return vuzpq_f32(a, b);
 #else
   const float32x4x2_t v;
   v.val[0] = {{a.val[0], a.val[2], b.val[0], b.val[2]}};
   v.val[1] = {{a.val[1], a.val[3], b.val[1], b.val[3]}};
   return v;
 #endif
 }
 
 /** linear interpolation
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 float32x2_linint(const float fr, const float32x2_t p0, const float32x2_t p1) {
   const float frinv = 1.f - fr;
 #if defined(NEON_SIMD_FP)
   return vadd_f32(vmul_n_f32(p0, frinv), vmul_n_f32(p1, fr));
 #else
   const float32x2_t v = {{frinv * p0.val[0] + fr * p1.val[0], frinv * p0.val[1] + fr * p1.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 float32x4_linint(const float fr, const float32x4_t p0, const float32x4_t p1) {
   const float frinv = 1.f - fr;
 #if defined(NEON_SIMD_FP)
   return vaddq_f32(vmulq_n_f32(p0, frinv), vmulq_n_f32(p1, fr));
 #else
   const float32x4_t v = {{frinv * p0.val[0] + fr * p1.val[0], frinv * p0.val[1] + fr * p1.val[1],
                           frinv * p0.val[2] + fr * p1.val[2], frinv * p0.val[3] + fr * p1.val[3]}};
   return v;
 #endif
 }
 
 /** Absolute value
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_fabsfx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   uint32x2_t xs = vreinterpret_u32_f32(x);
   return vreinterpret_f32_u32(vand_u32(xs, vdup_n_u32(0x7FFFFFFFU)));
 #else
   uf32x2_t xs = {x};
   xs.u.val[0] &= 0x7fffffff;
   xs.u.val[1] &= 0x7fffffff;
   return xs.f;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_fabsfx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   uint32x4_t xs = vreinterpretq_u32_f32(x);
   return vreinterpretq_f32_u32(vandq_u32(xs, vdupq_n_u32(0x7FFFFFFFU)));
 #else
   uf32x4_t xs = {x};
   xs.u.val[0] &= 0x7fffffff;
   xs.u.val[1] &= 0x7fffffff;
   xs.u.val[2] &= 0x7fffffff;
   xs.u.val[3] &= 0x7fffffff;
   return xs.f;
 #endif
 }
 
 /** Copy sign
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_copysignfx2(float32x2_t x, float32x2_t y) {
 #if defined(NEON_SIMD_FP)
   uint32x2_t xs = vreinterpret_u32_f32(x);
   uint32x2_t ys = vreinterpret_u32_f32(y);
   xs = vand_u32(xs, vdup_n_u32(0x7FFFFFFFU));
   return vreinterpret_f32_u32(vorr_u32(xs, vand_u32(ys, vdup_n_u32(0x80000000U))));
 #else
   uf32x2_t xs = {x};
   uf32x2_t ys = {y};
 
   xs.u.val[0] &= 0x7fffffff;
   xs.u.val[1] &= 0x7fffffff;
   xs.u.val[0] |= ys.u.val[0] & 0x80000000;
   xs.u.val[1] |= ys.u.val[1] & 0x80000000;
 
   return xs.f;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_copysignfx4(float32x4_t x, float32x4_t y) {
 #if defined(NEON_SIMD_FP)
   uint32x4_t xs = vreinterpretq_u32_f32(x);
   uint32x4_t ys = vreinterpretq_u32_f32(y);
   return vreinterpretq_f32_u32(vorrq_u32(vandq_u32(xs, vdupq_n_u32(0x7FFFFFFFU)), vandq_u32(ys, vdupq_n_u32(0x80000000U))));
 #else
   uf32x4_t xs = {x};
   uf32x4_t ys = {y};
 
   xs.u.val[0] &= 0x7fffffff;
   xs.u.val[1] &= 0x7fffffff;
   xs.u.val[2] &= 0x7fffffff;
   xs.u.val[3] &= 0x7fffffff;
   xs.u.val[0] |= ys.u.val[0] & 0x80000000;
   xs.u.val[1] |= ys.u.val[1] & 0x80000000;
   xs.u.val[2] |= ys.u.val[2] & 0x80000000;
   xs.u.val[3] |= ys.u.val[3] & 0x80000000;
 
   return xs.f;
 #endif
 }
 
 /** Truncate fractional part of float value
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_f32x2_trunc(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvt_f32_s32(vcvt_s32_f32(x));
 #else
   const float32x2_t tmp = {{(float)(uint32_t)x.val[0], (float)(uint32_t)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_f32x4_trunc(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvtq_f32_s32(vcvtq_s32_f32(x));
 #else
   const float32x4_t tmp = {{(float)(uint32_t)x.val[0], (float)(uint32_t)x.val[1], (float)(uint32_t)x.val[2], (float)(uint32_t)x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Convert to/from integer/float value
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 si_f32x2_to_u32x2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvt_u32_f32(x);
 #else
   const uint32x2_t tmp = {{(uint32_t)x.val[0], (uint32_t)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 si_f32x4_to_u32x4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvtq_u32_f32(x);
 #else
   const uint32x4_t tmp = {{(uint32_t)x.val[0], (uint32_t)x.val[1], (uint32_t)x.val[2], (uint32_t)x.val[3]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_u32x2_to_f32x2(uint32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvt_f32_u32(x);
 #else
   const float32x2_t tmp = {{(float)x.val[0], (float)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_u32x4_to_f32x4(uint32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvtq_f32_u32(x);
 #else
   const float32x4_t tmp = {{(float)x.val[0], (float)x.val[1], (float)x.val[2], (float)x.val[3]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 si_f32x2_to_i32x2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvt_s32_f32(x);
 #else
   const int32x2_t tmp = {{(int32_t)x.val[0], (int32_t)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 si_f32x4_to_i32x4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvtq_s32_f32(x);
 #else
   const int32x4_t tmp = {{(int32_t)x.val[0], (int32_t)x.val[1], (int32_t)x.val[2], (int32_t)x.val[3]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_i32x2_to_f32x2(int32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvt_f32_s32(x);
 #else
   const float32x2_t tmp = {{(float)x.val[0], (float)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_i32x4_to_f32x4(int32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvtq_f32_s32(x);
 #else
   const float32x4_t tmp = {{(float)x.val[0], (float)x.val[1], (float)x.val[2], (float)x.val[3]}};
   return tmp;
 #endif
 }

 // convert from int to float using variable q-representiation
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_i32x4qn_to_f32x4(int32x4_t x, int32_t qPoint) {
 #if defined(NEON_SIMD_FP)
   return vcvtq_n_f32_s32(x, qPoint);
 #else
   const float divisor = 1.f / (1 << qPoint);
   const float32x4_t tmp = {{(float)x.val[0] * divisor, (float)x.val[1] * divisor, (float)x.val[2] * divisor, (float)x.val[3] * divisor}};
   return tmp;
 #endif
 }

 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_i32x2qn_to_f32x2(int32x2_t x, int32_t qPoint) {
 #if defined(NEON_SIMD_FP)
   return vcvt_n_f32_s32(x, qPoint);
 #else
   const float divisor = 1.f / (1 << qPoint);
   const float32x2_t tmp = {{(float)x.val[0] * divisor, (float)x.val[1] * divisor}};
   return tmp;
 #endif
 }
 
 /** Reinterpret as integer/float value
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 si_f32x2_as_u32x2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vreinterpret_u32_f32(x);
 #else
   const uint32x2_t tmp = {{(uint32_t)x.val[0], (uint32_t)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 si_f32x4_as_u32x4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vreinterpretq_u32_f32(x);
 #else
   const uint32x4_t tmp = {{(uint32_t)x.val[0], (uint32_t)x.val[1], (uint32_t)x.val[2], (uint32_t)x.val[3]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_u32x2_as_f32x2(uint32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vreinterpret_f32_u32(x);
 #else
   const float32x2_t tmp = {{(float)x.val[0], (float)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_u32x4_as_f32x4(uint32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vreinterpretq_f32_u32(x);
 #else
   const float32x4_t tmp = {{(float)x.val[0], (float)x.val[1], (float)x.val[2], (float)x.val[3]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 si_f32x2_as_i32x2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vreinterpret_s32_f32(x);
 #else
   const int32x2_t tmp = {{(int32_t)x.val[0], (int32_t)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 si_f32x4_as_i32x4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vreinterpretq_s32_f32(x);
 #else
   const int32x4_t tmp = {{(int32_t)x.val[0], (int32_t)x.val[1], (int32_t)x.val[2], (int32_t)x.val[3]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_i32x2_as_f32x2(int32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vreinterpret_f32_s32(x);
 #else
   const float32x2_t tmp = {{(float)x.val[0], (float)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_i32x4_as_f32x4(int32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vreinterpretq_f32_s32(x);
 #else
   const float32x4_t tmp = {{(float)x.val[0], (float)x.val[1], (float)x.val[2], (float)x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Floor function
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_floorfx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvt_f32_s32(vcvt_s32_f32(x));
 #else
   const float32x2_t tmp = {{(float)(uint32_t)x.val[0], (float)(uint32_t)x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_floorfx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vcvtq_f32_s32(vcvtq_s32_f32(x));
 #else
   const float32x4_t tmp = {{(float)(uint32_t)x.val[0], (float)(uint32_t)x.val[1], (float)(uint32_t)x.val[2], (float)(uint32_t)x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Ceiling function
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_ceilfx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   const int32x2_t sign = vand_s32(vreinterpret_s32_f32(x), vdup_n_s32(-2147483648));
   const float32x2_t tmp_flt = vreinterpret_f32_s32(vorr_s32(vreinterpret_s32_f32(vdup_n_f32(1.f)), sign));
   return vcvt_f32_s32(vcvt_s32_f32(vadd_f32(x, tmp_flt)));
 #else
   const float32x2_t tmp = {{(float)((uint32_t)x.val[0] + 1), (float)((uint32_t)x.val[1] + 1)}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_ceilfx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   const int32x4_t sign = vandq_s32(vreinterpretq_s32_f32(x), vdupq_n_s32(-2147483648));
   const float32x4_t tmp_flt = vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(vdupq_n_f32(1.f)), sign));
   return vcvtq_f32_s32(vcvtq_s32_f32(vaddq_f32(x, tmp_flt)));
 #else
   const float32x4_t tmp = {{(float)((uint32_t)x.val[0] + 1), (float)((uint32_t)x.val[1] + 1), (float)((uint32_t)x.val[2] + 1), (float)((uint32_t)x.val[3] + 1)}};
   return tmp;
 #endif
 }
 
 /** Round to nearest integer.
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 si_roundfx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   const int32x2_t sign = vand_s32(vreinterpret_s32_f32(x), vdup_n_s32(-2147483648));
   const float32x2_t tmp_flt = vreinterpret_f32_s32(vorr_s32(vreinterpret_s32_f32(vdup_n_f32(0.5f)), sign));
   return vcvt_f32_s32(vcvt_s32_f32(vadd_f32(x, tmp_flt)));
 #else
   const float32x2_t tmp = {{roundf(x.val[0]), roundf(x.val[1])}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 si_roundfx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   const int32x4_t sign = vandq_s32(vreinterpretq_s32_f32(x), vdupq_n_s32(-2147483648));
   const float32x4_t tmp_flt = vreinterpretq_f32_s32(vorrq_s32(vreinterpretq_s32_f32(vdupq_n_f32(0.5f)), sign));
   return vcvtq_f32_s32(vcvtq_s32_f32(vaddq_f32(x, tmp_flt)));
 #else
   const float32x4_t tmp = {{roundf(x.val[0]), roundf(x.val[1]), roundf(x.val[2]), roundf(x.val[3])}};
   return tmp;
 #endif
 }
 
 /** Clip upper bound of x to m (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 clipmaxfx2(float32x2_t x, float32x2_t m) {
 #if defined(NEON_SIMD_FP)
   return vmin_f32(m, x);
   //const float32x2_t tmp = {(x[0] > m[0]) ? m[0] : x[0], (x[1] > m[1]) ? m[1] : x[1]};
   //return tmp;
 #else
   const float32x2_t tmp = {{(x.val[0] > m.val[0]) ? m.val[0] : x.val[0], (x.val[1] > m.val[1]) ? m.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 clipmaxfx4(float32x4_t x, float32x4_t m) {
 #if defined(NEON_SIMD_FP)
   return vminq_f32(m, x);
   /* const float32x4_t tmp = {(x[0] > m[0]) ? m[0] : x[0], (x[1] > m[1]) ? m[1] : x[1], */
   /*                          (x[2] > m[2]) ? m[2] : x[2], (x[3] > m[3]) ? m[3] : x[3]}; */
   /* return tmp; */
 #else
   const float32x4_t tmp = {{(x.val[0] > m.val[0]) ? m.val[0] : x.val[0], (x.val[1] > m.val[1]) ? m.val[1] : x.val[1],
                             (x.val[2] > m.val[2]) ? m.val[2] : x.val[2], (x.val[3] > m.val[3]) ? m.val[3] : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip lower bound of x to m (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 clipminfx2(float32x2_t m, float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vmax_f32(m, x);
   //const float32x2_t tmp = {(x[0] < m[0]) ? m[0] : x[0], (x[1] < m[1]) ? m[1] : x[1]};
   //return tmp;
 #else
   const float32x2_t tmp = {{(x.val[0] < m.val[0]) ? m.val[0] : x.val[0], (x.val[1] < m.val[1]) ? m.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 clipminfx4(float32x4_t m, float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vmaxq_f32(m, x);
   //const float32x4_t tmp = {(x[0] < m[0]) ? m[0] : x[0], (x[1] < m[1]) ? m[1] : x[1],
   //                         (x[2] < m[2]) ? m[2] : x[2], (x[3] < m[3]) ? m[3] : x[3]};
 //return tmp;
 #else
   const float32x4_t tmp = {{(x.val[0] < m.val[0]) ? m.val[0] : x.val[0], (x.val[1] < m.val[1]) ? m.val[1] : x.val[1],
                             (x.val[2] < m.val[2]) ? m.val[2] : x.val[2], (x.val[3] < m.val[3]) ? m.val[3] : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip x to min and max (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 clipminmaxfx2(float32x2_t min, float32x2_t x, float32x2_t max) {
 #if defined(NEON_SIMD_FP)
   return vmin_f32(max, vmax_f32(min, x));
   //const float32x2_t tmp = {(x[0] > max[0]) ? max[0] : (x[0] < min[0]) ? min[0] : x[0], (x[1] > max[1]) ? max[1] : (x[1] < min[1]) ? min[1] : x[1]};
   //return tmp;
 #else
   const float32x2_t tmp = {{(x.val[0] > max.val[0]) ? max.val[0] : (x.val[0] < min.val[0]) ? min.val[0] : x.val[0],
                             (x.val[1] > max.val[1]) ? max.val[1] : (x.val[1] < min.val[1]) ? min.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 clipminmaxfx4(float32x4_t min, float32x4_t x, float32x4_t max) {
 #if defined(NEON_SIMD_FP)
   return vminq_f32(max, vmaxq_f32(min, x));
   // TODO: implement using selection instructions
   //const float32x4_t tmp = {(x[0] > max[0]) ? max[0] : (x[0] < min[0]) ? min[0] : x[0], (x[1] > max[1]) ? max[1] : (x[1] < min[1]) ? min[1] : x[1],
   //                         (x[2] > max[2]) ? max[2] : (x[2] < min[2]) ? min[2] : x[2], (x[3] > max[3]) ? max[3] : (x[3] < min[3]) ? min[3] : x[3]};
   //return tmp;
 #else
   const float32x4_t tmp = {{(x.val[0] > max.val[0]) ? max.val[0] : (x.val[0] < min.val[0]) ? min.val[0] : x.val[0],
                             (x.val[1] > max.val[1]) ? max.val[1] : (x.val[1] < min.val[1]) ? min.val[1] : x.val[1],
                             (x.val[2] > max.val[2]) ? max.val[2] : (x.val[2] < min.val[2]) ? min.val[2] : x.val[2],
                             (x.val[3] > max.val[3]) ? max.val[3] : (x.val[3] < min.val[3]) ? min.val[3] : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip lower bound of x to 0.f (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 clip0fx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vmax_f32(vdup_n_f32(0.f), x);
   //const float32x2_t tmp = {(x[0] < 0.f) ? 0.f : x[0], (x[1] < 0.f) ? 0.f : x[1]};
   //return tmp;
 #else
   const float32x2_t tmp = {{(x.val[0] < 0.f) ? 0.f : x.val[0], (x.val[1] < 0.f) ? 0.f : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 clip0fx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vmaxq_f32(vdupq_n_f32(0.f), x);
   //const float32x4_t tmp = {(x[0] < 0.f) ? 0.f : x[0], (x[1] < 0.f) ? 0.f : x[1],
   //                         (x[2] < 0.f) ? 0.f : x[2], (x[3] < 0.f) ? 0.f : x[3]};
   //return tmp;
 #else
   const float32x4_t tmp = {{(x.val[0] < 0.f) ? 0.f : x.val[0], (x.val[1] < 0.f) ? 0.f : x.val[1],
                             (x.val[2] < 0.f) ? 0.f : x.val[2], (x.val[3] < 0.f) ? 0.f : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip upper bound of x to 1.f (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 clip1fx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vmin_f32(vdup_n_f32(1.f), x);
   //const float32x2_t tmp = {(x[0] > 1.f) ? 1.f : x[0], (x[1] > 1.f) ? 1.f : x[1]};
   //return tmp;
 #else
   const float32x2_t tmp = {{(x.val[0] > 1.f) ? 1.f : x.val[0], (x.val[1] > 1.f) ? 1.f : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 clip1fx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vminq_f32(vdupq_n_f32(1.f), x);
   //const float32x4_t tmp = {(x[0] > 1.f) ? 1.f : x[0], (x[1] > 1.f) ? 1.f : x[1],
   //                         (x[2] > 1.f) ? 1.f : x[2], (x[3] > 1.f) ? 1.f : x[3]};
   //return tmp;
 #else
   const float32x4_t tmp = {{(x.val[0] > 1.f) ? 1.f : x.val[0], (x.val[1] > 1.f) ? 1.f : x.val[1],
                             (x.val[2] > 1.f) ? 1.f : x.val[2], (x.val[3] > 1.f) ? 1.f : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip x to [0.f, 1.f] (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 clip01fx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vmin_f32(vdup_n_f32(1.f), vmax_f32(vdup_n_f32(0.f), x));
   //const float32x2_t tmp = {(x[0] > 1.f) ? 1.f : (x[0] < 0.f) ? 0.f : x[0], (x[1] > 1.f) ? 1.f : (x[1] < 0.f) ? 0.f : x[1]};
   //return tmp;
 #else
   const float32x2_t tmp = {{(x.val[0] > 1.f) ? 1.f : (x.val[0] < 0.f) ? 0.f : x.val[0],
                             (x.val[1] > 1.f) ? 1.f : (x.val[1] < 0.f) ? 0.f : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 clip01fx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vminq_f32(vdupq_n_f32(1.f), vmaxq_f32(vdupq_n_f32(0.f), x));
   //const float32x4_t tmp = {(x[0] > 1.f) ? 1.f : (x[0] < 0.f) ? 0.f : x[0], (x[1] > 1.f) ? 1.f : (x[1] < 0.f) ? 0.f : x[1],
   //                         (x[2] > 1.f) ? 1.f : (x[2] < 0.f) ? 0.f : x[2], (x[3] > 1.f) ? 1.f : (x[3] < 0.f) ? 0.f : x[1]};
   //return tmp;
 #else
   const float32x4_t tmp = {{(x.val[0] > 1.f) ? 1.f : (x.val[0] < 0.f) ? 0.f : x.val[0],
                             (x.val[1] > 1.f) ? 1.f : (x.val[1] < 0.f) ? 0.f : x.val[1],
                             (x.val[2] > 1.f) ? 1.f : (x.val[2] < 0.f) ? 0.f : x.val[2],
                             (x.val[3] > 1.f) ? 1.f : (x.val[3] < 0.f) ? 0.f : x.val[1]}};
   return tmp;
 #endif
 }
 
 /** Clip lower bound of x to -1.f (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 clipm1fx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vmax_f32(vdup_n_f32(-1.f), x);
   //const float32x2_t tmp = {(x[0] < -1.f) ? -1.f : x[0], (x[1] < -1.f) ? -1.f : x[1]};
   //return tmp;
 #else
   const float32x2_t tmp = {{(x.val[0] < -1.f) ? -1.f : x.val[0], (x.val[1] < -1.f) ? -1.f : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 clipm1fx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vmaxq_f32(vdupq_n_f32(-1.f), x);
   //const float32x4_t tmp = {(x[0] < -1.f) ? -1.f : x[0], (x[1] < -1.f) ? -1.f : x[1],
   //                         (x[2] < -1.f) ? -1.f : x[2], (x[3] < -1.f) ? -1.f : x[3]};
   //return tmp;
 #else
   const float32x4_t tmp = {{(x.val[0] < -1.f) ? -1.f : x.val[0], (x.val[1] < -1.f) ? -1.f : x.val[1],
                             (x.val[2] < -1.f) ? -1.f : x.val[2], (x.val[3] < -1.f) ? -1.f : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip x to [-1.f, 1.f] (inclusive) -- upper inclusive bound not respected by current simd optimization
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 clip1m1fx2(float32x2_t x) {
 #if defined(NEON_SIMD_FP)
   return vmin_f32(vdup_n_f32(1.f), vmax_f32(vdup_n_f32(-1.f), x));
   //const float32x2_t tmp = {(x[0] > 1.f) ? 1.f : (x[0] < -1.f) ? -1.f : x[0], (x[1] > 1.f) ? 1.f : (x[1] < -1.f) ? -1.f : x[1]};
   //return tmp;
 #else
   const float32x2_t tmp = {{(x.val[0] > 1.f) ? 1.f : (x.val[0] < -1.f) ? -1.f : x.val[0],
                             (x.val[1] > 1.f) ? 1.f : (x.val[1] < -1.f) ? -1.f : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 clip1m1fx4(float32x4_t x) {
 #if defined(NEON_SIMD_FP)
   return vminq_f32(vdupq_n_f32(1.f), vmaxq_f32(vdupq_n_f32(-1.f), x));
   //const float32x4_t tmp = {(x[0] > 1.f) ? 1.f : (x[0] < -1.f) ? -1.f : x[0], (x[1] > 1.f) ? 1.f : (x[1] < -1.f) ? -1.f : x[1],
   //                         (x[2] > 1.f) ? 1.f : (x[2] < -1.f) ? -1.f : x[2], (x[3] > 1.f) ? 1.f : (x[3] < -1.f) ? -1.f : x[3]};
   //return tmp;
 #else
   const float32x4_t tmp = {{(x.val[0] > 1.f) ? 1.f : (x.val[0] < -1.f) ? -1.f : x.val[0],
                             (x.val[1] > 1.f) ? 1.f : (x.val[1] < -1.f) ? -1.f : x.val[1],
                             (x.val[2] > 1.f) ? 1.f : (x.val[2] < -1.f) ? -1.f : x.val[2],
                             (x.val[3] > 1.f) ? 1.f : (x.val[3] < -1.f) ? -1.f : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** @} */
 
 /*===========================================================================*/
 /* Interpolation.                                                            */
 /*===========================================================================*/
 
 /**
  * @name    Interpolations
  * @todo    Add cubic/spline interpolations
  * @{
  */
 
 /** Linear interpolation
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x2_t
 linintfx2(const float32x2_t fr, const float32x2_t x0, const float32x2_t x1) {
   return float32x2_fmuladd(x0, fr, float32x2_sub(x1, x0));
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 float32x4_t
 linintfx4(const float32x4_t fr, const float32x4_t x0, const float32x4_t x1) {
   return float32x4_fmuladd(x0, fr, float32x4_sub(x1, x0));
 }
 
 /** Cosine interpolation TODO
  */
 /* static inline __attribute__((optimize("Ofast"), always_inline)) float cosintf(const float fr, const float x0, const float x1) { */
 /*   const float tmp = (1.f - fastercosfullf(fr * M_PI)) * 0.5f; */
 /*   return x0 + tmp * (x1 - x0); */
 /* } */
 
 /** @} */
 
 #endif  // __mk2_float_simd_h
 