/**
 * @file    mk2_int_simd.h
 * @brief   SIMD Integral Utilities.
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_int_simd Integral SIMD utilities
 * @{
 *
 */

 #ifndef __mk2_int_simd_h
 #define __mk2_int_simd_h
 
 #include <math.h>
 #include <stdint.h>
 
 #if defined(__ARM_NEON)
 #include <arm_neon.h>
 #define NEON_SIMD_INT 1
 #endif
 
 /*===========================================================================*/
 /* Types.                                                                    */
 /*===========================================================================*/
 
 /**
  * @name    Types
  * @{
  */
 
 //  8-bit integrals --------------------------------------------------------------
 
 #if !defined(NEON_SIMD_INT)
 /* typedef int8_t int8x8_t[8] __attribute__((aligned(4))); */
 typedef struct int8x8 {
   int8_t val[8];
 } int8x8_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int8x8_t
 int8x8(const int8_t a, const int8_t b, const int8_t c, const int8_t d,
        const int8_t e, const int8_t f, const int8_t g, const int8_t h) {
 #if defined(NEON_SIMD_INT)
   const int8x8_t v = {a, b, c, d, e, f, g, h};
   return v;
 #else
   const int8x8_t v = {{a, b, c, d, e, f, g, h}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef uint8_t uint8x8_t[8] __attribute__((aligned(4))); */
 typedef struct uint8x8 {
   uint8_t val[8];
 } uint8x8_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint8x8_t
 uint8x8(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d,
         const uint8_t e, const uint8_t f, const uint8_t g, const uint8_t h) {
 #if defined(NEON_SIMD_INT)
   const uint8x8_t v = {a, b, c, d, e, f, g, h};
   return v;
 #else
   const uint8x8_t v = {{a, b, c, d, e, f, g, h}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef int8_t int8x16_t[16] __attribute__((aligned(4))); */
 typedef struct int8x16 {
   int8_t val[16];
 } int8x16_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int8x16_t
 int8x16(const int8_t a, const int8_t b, const int8_t c, const int8_t d,
         const int8_t e, const int8_t f, const int8_t g, const int8_t h,
         const int8_t i, const int8_t j, const int8_t k, const int8_t l,
         const int8_t m, const int8_t n, const int8_t o, const int8_t p) {
 #if defined(NEON_SIMD_INT)
   const int8x16_t v = {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p};
   return v;
 #else
   const int8x16_t v = {{a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef uint8_t uint8x16_t[16] __attribute__((aligned(4))); */
 typedef struct uint8x16 {
   uint8_t val[16];
 } uint8x16_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint8x16_t
 uint8x16(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d,
          const uint8_t e, const uint8_t f, const uint8_t g, const uint8_t h,
          const uint8_t i, const uint8_t j, const uint8_t k, const uint8_t l,
          const uint8_t m, const uint8_t n, const uint8_t o, const uint8_t p) {
 #if defined(NEON_SIMD_INT)
   const uint8x16_t v = {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p};
   return v;
 #else
   const uint8x16_t v = {{a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p}};
   return v;
 #endif
 }
 
 #if defined(NEON_SIMD_INT)
 #define i8x8_lane(u, i) ((u)[(i)])
 #define u8x8_lane(u, i) ((u)[(i)])
 #define i8x16_lane(u, i) ((u)[(i)])
 #define u8x16_lane(u, i) ((u)[(i)])
 #else
 #define i8x8_lane(u, i) ((u).val[(i)])
 #define u8x8_lane(u, i) ((u).val[(i)])
 #define i8x16_lane(u, i) ((u).val[(i)])
 #define u8x16_lane(u, i) ((u).val[(i)])
 #endif
 
 #if defined(NEON_SIMD_INT)
 #define i8x8_const(c) \
   { (c), (c), (c), (c), (c), (c), (c), (c) }
 #define u8x8_const(c) \
   { (c), (c), (c), (c), (c), (c), (c), (c) }
 #define i8x16_const(c) \
   { (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c) }
 #define u8x16_const(c) \
   { (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c) }
 #else
 #define i8x8_const(c)                          \
   {                                            \
     { (c), (c), (c), (c), (c), (c), (c), (c) } \
   }
 #define u8x8_const(c)                          \
   {                                            \
     { (c), (c), (c), (c), (c), (c), (c), (c) } \
   }
 #define i8x16_const(c)                                                                 \
   {                                                                                    \
     { (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c) } \
   }
 #define u8x16_const(c)                                                                 \
   {                                                                                    \
     { (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c), (c) } \
   }
 #endif
 
 // 16-bit integrals --------------------------------------------------------------
 
 #if !defined(NEON_SIMD_INT)
 /* typedef int16_t int16x4_t[4] __attribute__((aligned(4))); */
 typedef struct int16x4 {
   int16_t val[4];
 } int16x4_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int16x4_t
 int16x4(const int16_t a, const int16_t b, const int16_t c, const int16_t d) {
 #if defined(NEON_SIMD_INT)
   const int16x4_t v = {a, b, c, d};
   return v;
 #else
   const int16x4_t v = {{a, b, c, d}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef uint16_t uint16x4_t[4] __attribute__((aligned(4))); */
 typedef struct uint16x4 {
   uint16_t val[4];
 } uint16x4_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint16x4_t
 uint16x4(const uint16_t a, const uint16_t b, const uint16_t c, const uint16_t d) {
 #if defined(NEON_SIMD_INT)
   const uint16x4_t v = {a, b, c, d};
   return v;
 #else
   const uint16x4_t v = {{a, b, c, d}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef int16_t int16x8_t[8] __attribute__((aligned(4))); */
 typedef struct int16x8 {
   int16_t val[8];
 } int16x8_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int16x8_t
 int16x8(const int16_t a, const int16_t b, const int16_t c, const int16_t d,
         const int16_t e, const int16_t f, const int16_t g, const int16_t h) {
 #if defined(NEON_SIMD_INT)
   const int16x8_t v = {a, b, c, d, e, f, g, h};
   return v;
 #else
   const int16x8_t v = {{a, b, c, d, e, f, g, h}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef uint16_t uint16x8_t[8] __attribute__((aligned(4))); */
 typedef struct uint16x8 {
   uint16_t val[8];
 } uint16x8_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint16x8_t
 uint16x8(const uint16_t a, const uint16_t b, const uint16_t c, const uint16_t d,
          const uint16_t e, const uint16_t f, const uint16_t g, const uint16_t h) {
 #if defined(NEON_SIMD_INT)
   const uint16x8_t v = {a, b, c, d, e, f, g, h};
   return v;
 #else
   const uint16x8_t v = {{a, b, c, d, e, f, g, h}};
   return v;
 #endif
 }
 
 #if defined(NEON_SIMD_INT)
 #define i16x4_lane(u, i) ((u)[(i)])
 #define u16x4_lane(u, i) ((u)[(i)])
 #define i16x8_lane(u, i) ((u)[(i)])
 #define u16x8_lane(u, i) ((u)[(i)])
 #else
 #define i16x4_lane(u, i) ((u).val[(i)])
 #define u16x4_lane(u, i) ((u).val[(i)])
 #define i16x8_lane(u, i) ((u).val[(i)])
 #define u16x8_lane(u, i) ((u).val[(i)])
 #endif
 
 #if defined(NEON_SIMD_INT)
 #define i16x4_const(c) \
   { (c), (c), (c), (c) }
 #define u16x4_const(c) \
   { (c), (c), (c), (c) }
 #define i16x8_const(c) \
   { (c), (c), (c), (c), (c), (c), (c), (c) }
 #define u16x8_const(c) \
   { (c), (c), (c), (c), (c), (c), (c), (c) }
 #else
 #define i16x4_const(c)     \
   {                        \
     { (c), (c), (c), (c) } \
   }
 #define u16x4_const(c)     \
   {                        \
     { (c), (c), (c), (c) } \
   }
 #define i16x8_const(c)                         \
   {                                            \
     { (c), (c), (c), (c), (c), (c), (c), (c) } \
   }
 #define u16x8_const(c)                         \
   {                                            \
     { (c), (c), (c), (c), (c), (c), (c), (c) } \
   }
 #endif
 
 // 32-bit integrals --------------------------------------------------------------
 
 #if !defined(NEON_SIMD_INT)
 /* typedef int32_t int32x2_t[2] __attribute__((aligned(4))); */
 typedef struct int32x2 {
   int32_t val[2];
 } int32x2_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2(const int32_t a, const int32_t b) {
 #if defined(NEON_SIMD_INT)
   const int32x2_t v = {a, b};
   return v;
 #else
   const int32x2_t v = {{a, b}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef uint32_t uint32x2_t[2] __attribute__((aligned(4))); */
 typedef struct uint32x2 {
   uint32_t val[2];
 } uint32x2_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2(const uint32_t a, const uint32_t b) {
 #if defined(NEON_SIMD_INT)
   const uint32x2_t v = {a, b};
   return v;
 #else
   const uint32x2_t v = {{a, b}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef int32_t int32x4_t[4] __attribute__((aligned(4))); */
 typedef struct int32x4 {
   int32_t val[4];
 } int32x4_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4(const int32_t a, const int32_t b, const int32_t c, const int32_t d) {
 #if defined(NEON_SIMD_INT)
   const int32x4_t v = {a, b, c, d};
   return v;
 #else
   const int32x4_t v = {{a, b, c, d}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 /* typedef uint32_t uint32x4_t[4] __attribute__((aligned(4))); */
 typedef struct uint32x4 {
   uint32_t val[4];
 } uint32x4_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4(const uint32_t a, const uint32_t b, const uint32_t c, const uint32_t d) {
 #if defined(NEON_SIMD_INT)
   const uint32x4_t v = {a, b, c, d};
   return v;
 #else
   const uint32x4_t v = {{a, b, c, d}};
   return v;
 #endif
 }
 
 #if defined(NEON_SIMD_INT)
 #define i32x2_lane(u, i) ((u)[(i)])
 #define u32x2_lane(u, i) ((u)[(i)])
 #define i32x4_lane(u, i) ((u)[(i)])
 #define u32x4_lane(u, i) ((u)[(i)])
 #else
 #define i32x2_lane(u, i) ((u).val[(i)])
 #define u32x2_lane(u, i) ((u).val[(i)])
 #define i32x4_lane(u, i) ((u).val[(i)])
 #define u32x4_lane(u, i) ((u).val[(i)])
 #endif
 
 #if defined(NEON_SIMD_INT)
 #define i32x2_const(c) \
   { (c), (c) }
 #define u32x2_const(c) \
   { (c), (c) }
 #define i32x4_const(c) \
   { (c), (c), (c), (c) }
 #define u32x4_const(c) \
   { (c), (c), (c), (c) }
 #else
 #define i32x2_const(c) \
   {                    \
     { (c), (c) }       \
   }
 #define u32x2_const(c) \
   {                    \
     { (c), (c) }       \
   }
 #define i32x4_const(c)     \
   {                        \
     { (c), (c), (c), (c) } \
   }
 #define u32x4_const(c)     \
   {                        \
     { (c), (c), (c), (c) } \
   }
 #endif
 
 // Compound 32-bit integrals -----------------------------------------------------
 
 #if !defined(NEON_SIMD_INT)
 typedef struct {
   int32x2_t val[2];
 } int32x2x2_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2x2_t
 int32x2x2(const int32x2_t a, const int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   const int32x2x2_t v = {{a, b}};
   return v;
 #else
   const int32x2x2_t v = {{a, b}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 typedef struct {
   uint32x2_t val[2];
 } uint32x2x2_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2x2_t
 uint32x2x2(const uint32x2_t a, const uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   const uint32x2x2_t v = {{a, b}};
   return v;
 #else
   const uint32x2x2_t v = {{a, b}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 typedef struct {
   int32x4_t val[2];
 } int32x4x2_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4x2_t
 int32x4x2(const int32x4_t a, const int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   const int32x4x2_t v = {{a, b}};
   return v;
 #else
   const int32x4x2_t v = {{a, b}};
   return v;
 #endif
 }
 
 #if !defined(NEON_SIMD_INT)
 typedef struct {
   uint32x4_t val[2];
 } uint32x4x2_t __attribute__((aligned(4)));
 #endif
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4x2_t
 uint32x4x2(const uint32x4_t a, const uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   const uint32x4x2_t v = {{a, b}};
   return v;
 #else
   const uint32x4x2_t v = {{a, b}};
   return v;
 #endif
 }
 
 /** @} */
 
 /*===========================================================================*/
 /* SIMD Operations.                                                          */
 /*===========================================================================*/
 
 /**
  * @name    SIMD Operations
  * @todo    Add more wrappers if needed. Full coverage would be quite tedious so if in depth
  *          optimization is required consider checking for NEON_SIMD_INT and use NEON intrinsics
  *          directly
  * @{
  */
 
 #if defined(NEON_SIMD_INT)
 #define s32x2_ld(ptr) vld1_s32((ptr))
 #define s32x4_ld(ptr) vld1q_s32((ptr))
 /* #define s32x2x2_ld(ptr) vld1_s32_x2((ptr)) */  // Missing from GCC
 // Note: must make sure to pass int32x2x2 as v
 #define s32x2x2_ld(ptr, v) asm volatile("\tvld1.32 %h0, [%1]\n" \
                                         : "=w"((v))             \
                                         : "r"((ptr))            \
                                         :)
 /* #define s32x4x2_ld(ptr) vld1q_s32_x2((ptr)) */  // Missing from GCC
 // Note: must make sure to pass int32x4x2 as v
 #define s32x4x2_ld(ptr, v) asm volatile("\tvld1.32 %h0, [%1]\n" \
                                         : "=w"((v))             \
                                         : "r"((ptr))            \
                                         :)
 #define s32x2x2_ld2(ptr) vld2_s32((ptr))   // interleaving type 2
 #define s32x4x2_ld2(ptr) vld2q_s32((ptr))  // interleaving type 2
 #define s32x2_str(ptr, v) vst1_s32((ptr), (v))
 #define s32x4_str(ptr, v) vst1q_s32((ptr), (v))
 /* #define s32x2x2_str(ptr, v) vst1_s32_x2((ptr), (v)) */  // Missing from GCC
 // Note: must make sure to pass int32x2x2 as v
 #define s32x2x2_str(ptr, v) asm volatile("\tvst1.32 %h0, [%1]!\n" \
                                          :                        \
                                          : "w"((v)), "r"((ptr))   \
                                          :)
 /* #define s32x4x2_str(ptr, v) vst1q_s32_x2((ptr), (v)) */  // Missing from GCC
 // Note: must make sure to pass int32x4x2 as v
 #define s32x4x2_str(ptr, v) asm volatile("\tvst1.32 %h0, [%1]!\n" \
                                          :                        \
                                          : "w"((v)), "r"((ptr))   \
                                          :)
 #define s32x2x2_str2(ptr, v) vst2_s32((ptr), (v))   // interleaving type 2
 #define s32x4x2_str2(ptr, v) vst2q_s32((ptr), (v))  // interleaving type 2
 #define s32x2_dup(c) vdup_n_s32((c))
 #define s32x4_dup(c) vdupq_n_s32((c))
 
 #define u32x2_ld(ptr) vld1_u32((ptr))
 #define u32x4_ld(ptr) vld1q_u32((ptr))
 /* #define u32x2x2_ld(ptr) vld1_u32_x2((ptr)) */  // Missing from GCC
 // Note: must make sure to pass int32x2x2 as v
 #define u32x2x2_ld(ptr, v) asm volatile("\tvld1.32 %h0, [%1]\n" \
                                         : "=w"((v))             \
                                         : "r"((ptr))            \
                                         :)
 /* #define u32x4x2_ld(ptr) vld1q_u32_x2((ptr)) */  // Missing from GCC
 // Note: must make sure to pass int32x4x2 as v
 #define u32x4x2_ld(ptr, v) asm volatile("\tvld1.32 %h0, [%1]\n" \
                                         : "=w"((v))             \
                                         : "r"((ptr))            \
                                         :)
 #define u32x2x2_ld2(ptr) vld2_u32((ptr))   // interleaving type 2
 #define u32x4x2_ld2(ptr) vld2q_u32((ptr))  // interleaving type 2
 #define u32x2_str(ptr, v) vst1_u32((ptr), (v))
 #define u32x4_str(ptr, v) vst1q_u32((ptr), (v))
 /* #define u32x2x2_str(ptr, v) vst1_u32_x2((ptr), (v)) */  // Missing from GCC
 // Note: must make sure to pass uint32x2x2 as v
 #define u32x2x2_str(ptr, v) asm volatile("\tvst1.32 %h0, [%1]!\n" \
                                          :                        \
                                          : "w"((v)), "r"((ptr))   \
                                          :)
 /* #define u32x4x2_str(ptr, v) vst1q_u32_x2((ptr), (v)) */  // Missing from GCC
 // Note: must make sure to pass uint32x4x2 as v
 #define u32x4x2_str(ptr, v) asm volatile("\tvst1.32 %h0, [%1]!\n" \
                                          :                        \
                                          : "w"((v)), "r"((ptr))   \
                                          :)
 #define u32x2x2_str2(ptr, v) vst2_u32((ptr), (v))   // interleaving type 2
 #define u32x4x2_str2(ptr, v) vst2q_u32((ptr), (v))  // interleaving type 2
 #define u32x2_dup(c) vdup_n_u32((c))
 #define u32x4_dup(c) vdupq_n_u32((c))
 // TODO: add more dup variants, with inline asm if needed
 #else
 #define s32x2_ld(ptr) (*(const int32x2_t *)(ptr))
 #define s32x4_ld(ptr) (*(const int32x4_t *)(ptr))
 #define s32x2x2_ld(ptr) (*(const int32x2x2_t *)(ptr))
 #define s32x4x2_ld(ptr) (*(const int32x4x2_t *)(ptr))
 #define s32x2_str(ptr, v)      \
   do {                         \
     *(int32x2_t *)(ptr) = (v); \
   } while (0);
 #define s32x4_str(ptr, v)     \
   do {                        \
     *(int32x4_t *)(ptr) = (v) \
   } while (0);
 #define s32x2x2_str(ptr, v)     \
   do {                          \
     *(int32x2x2_t *)(ptr) = (v) \
   } while (0);
 #define s32x4x2_str(ptr, v)     \
   do {                          \
     *(int32x4x2_t *)(ptr) = (v) \
   } while (0);
 #define s32x2x2_str2(ptr, v)          \
   do {                                \
     (ptr)[0] = (int32x4_t)(v).val[0]; \
     (ptr)[1] = (int32x4_t)(v).val[2]; \
     (ptr)[2] = (int32x4_t)(v).val[1]; \
     (ptr)[3] = (int32x4_t)(v).val[3]; \
   } while (0);
 #define s32x4x2_str2(ptr, v)                   \
   do {                                         \
     (ptr)[0] = (int32x4x2_t)(v).val[0].val[0]; \
     (ptr)[1] = (int32x4x2_t)(v).val[1].val[0]; \
     (ptr)[2] = (int32x4x2_t)(v).val[0].val[1]; \
     (ptr)[3] = (int32x4x2_t)(v).val[1].val[1]; \
     (ptr)[4] = (int32x4x2_t)(v).val[0].val[2]; \
     (ptr)[5] = (int32x4x2_t)(v).val[1].val[2]; \
     (ptr)[6] = (int32x4x2_t)(v).val[0].val[3]; \
     (ptr)[7] = (int32x4x2_t)(v).val[1].val[3]; \
   } while (0);
 #define s32x2_dup(c) int32x2((c), (c))
 #define s32x4_dup(c) int32x4((c), (c), (c), (c))
 
 #define u32x2_ld(ptr) (*(const uint32x2_t *)(ptr))
 #define u32x4_ld(ptr) (*(const uint32x4_t *)(ptr))
 #define u32x2x2_ld(ptr) (*(const uint32x2x2_t *)(ptr))
 #define u32x4x2_ld(ptr) (*(const uint32x4x2_t *)(ptr))
 #define u32x2_str(ptr, v)       \
   do {                          \
     *(uint32x2_t *)(ptr) = (v); \
   } while (0);
 #define u32x4_str(ptr, v)      \
   do {                         \
     *(uint32x4_t *)(ptr) = (v) \
   } while (0);
 #define u32x2x2_str(ptr, v)      \
   do {                           \
     *(uint32x2x2_t *)(ptr) = (v) \
   } while (0);
 #define u32x4x2_str(ptr, v)      \
   do {                           \
     *(uint32x4x2_t *)(ptr) = (v) \
   } while (0);
 #define u32x2x2_str2(ptr, v)           \
   do {                                 \
     (ptr)[0] = (uint32x4_t)(v).val[0]; \
     (ptr)[1] = (uint32x4_t)(v).val[2]; \
     (ptr)[2] = (uint32x4_t)(v).val[1]; \
     (ptr)[3] = (uint32x4_t)(v).val[3]; \
   } while (0);
 #define u32x4x2_str2(ptr, v)                    \
   do {                                          \
     (ptr)[0] = (uint32x4x2_t)(v).val[0].val[0]; \
     (ptr)[1] = (uint32x4x2_t)(v).val[1].val[0]; \
     (ptr)[2] = (uint32x4x2_t)(v).val[0].val[1]; \
     (ptr)[3] = (uint32x4x2_t)(v).val[1].val[1]; \
     (ptr)[4] = (uint32x4x2_t)(v).val[0].val[2]; \
     (ptr)[5] = (uint32x4x2_t)(v).val[1].val[2]; \
     (ptr)[6] = (uint32x4x2_t)(v).val[0].val[3]; \
     (ptr)[7] = (uint32x4x2_t)(v).val[1].val[3]; \
   } while (0);
 #define u32x2_dup(c) uint32x2((c), (c))
 #define u32x4_dup(c) uint32x4((c), (c), (c), (c))
 
 #endif
 
 /** Cast / reinterpret
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_to_uint32x2(int32x2_t p) {
 #if defined(NEON_SIMD_INT)
   return vreinterpret_u32_s32(p);
 #else
   return *(uint32x2_t *)(&p);
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 uint32x2_to_int32x2(uint32x2_t p) {
 #if defined(NEON_SIMD_INT)
   return vreinterpret_s32_u32(p);
 #else
   return *(int32x2_t *)(&p);
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_to_uint32x4(int32x4_t p) {
 #if defined(NEON_SIMD_INT)
   return vreinterpretq_u32_s32(p);
 #else
   return *(uint32x4_t *)(&p);
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 uint32x4_to_int32x4(uint32x4_t p) {
 #if defined(NEON_SIMD_INT)
   return vreinterpretq_s32_u32(p);
 #else
   return *(int32x4_t *)(&p);
 #endif
 }
 
 /** Addition
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_add(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vadd_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] + b.val[0], a.val[1] + b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_add(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vaddq_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] + b.val[0], a.val[1] + b.val[1], a.val[2] + b.val[2], a.val[3] + b.val[3]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_add(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vadd_s32(a, b);
 #else
   const int32x2_t v = {{a.val[0] + b.val[0], a.val[1] + b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_add(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vaddq_s32(a, b);
 #else
   const int32x4_t v = {{a.val[0] + b.val[0], a.val[1] + b.val[1], a.val[2] + b.val[2], a.val[3] + b.val[3]}};
   return v;
 #endif
 }
 
 /** Subtraction
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_sub(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vsub_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] + b.val[0], a.val[1] + b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_sub(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vsubq_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] + b.val[0], a.val[1] + b.val[1], a.val[2] + b.val[2], a.val[3] + b.val[3]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_sub(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vsub_s32(a, b);
 #else
   const int32x2_t v = {{a.val[0] + b.val[0], a.val[1] + b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_sub(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vsubq_s32(a, b);
 #else
   const int32x4_t v = {{a.val[0] + b.val[0], a.val[1] + b.val[1], a.val[2] + b.val[2], a.val[3] + b.val[3]}};
   return v;
 #endif
 }
 
 /** Multiplication
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_mul(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vmul_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] * b.val[0], a.val[1] * b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_mul(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vmulq_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] * b.val[0], a.val[1] * b.val[1], a.val[2] * b.val[2], a.val[3] * b.val[3]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_mul(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vmul_s32(a, b);
 #else
   const int32x2_t v = {{a.val[0] * b.val[0], a.val[1] * b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_mul(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vmulq_s32(a, b);
 #else
   const int32x4_t v = {{a.val[0] * b.val[0], a.val[1] * b.val[1], a.val[2] * b.val[2], a.val[3] * b.val[3]}};
   return v;
 #endif
 }
 
 /** Multiplication by scalar
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_mulscal(uint32x2_t a, uint32_t b) {
 #if defined(NEON_SIMD_INT)
   return vmul_n_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] * b, a.val[1] * b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_mulscal(uint32x4_t a, uint32_t b) {
 #if defined(NEON_SIMD_INT)
   return vmulq_n_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] * b, a.val[1] * b, a.val[2] * b, a.val[3] * b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_mulscal(int32x2_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vmul_n_s32(a, b);
 #else
   const int32x2_t v = {{a.val[0] * b, a.val[1] * b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_mulscal(int32x4_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vmulq_n_s32(a, b);
 #else
   const int32x4_t v = {{a.val[0] * b, a.val[1] * b, a.val[2] * b, a.val[3] * b}};
   return v;
 #endif
 }
 
 /** Shift left
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_shl(uint32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vshl_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] << b.val[0], a.val[1] << b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_shl(uint32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vshlq_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] << b.val[0], a.val[1] << b.val[1], a.val[2] << b.val[2], a.val[3] << b.val[3]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_shl(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vshl_s32(a, b);
 #else
   const int32x2_t v = {{a.val[0] << b.val[0], a.val[1] << b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_shl(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vshlq_s32(a, b);
 #else
   const int32x4_t v = {{a.val[0] << b.val[0], a.val[1] << b.val[1], a.val[2] << b.val[2], a.val[3] << b.val[3]}};
   return v;
 #endif
 }
 
 /** Shift left scalar
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_shlscal(uint32x2_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vshl_n_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] << b, a.val[1] << b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_shlscal(uint32x4_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vshlq_n_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] << b, a.val[1] << b, a.val[2] << b, a.val[3] << b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_shlscal(int32x2_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vshl_n_s32(a, b);
 #else
   const int32x2_t v = {{a.val[0] << b, a.val[1] << b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_shlscal(int32x4_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vshlq_n_s32(a, b);
 #else
   const int32x4_t v = {{a.val[0] << b, a.val[1] << b, a.val[2] << b, a.val[3] << b}};
   return v;
 #endif
 }
 
 /** Shift right scalar
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_shrscal(uint32x2_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vshr_n_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] >> b, a.val[1] >> b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_shrscal(uint32x4_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vshrq_n_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] >> b, a.val[1] >> b, a.val[2] >> b, a.val[3] >> b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_shrscal(int32x2_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vshr_n_s32(a, b);
 #else
   const int32x2_t v = {{a.val[0] >> b, a.val[1] >> b}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_shrscal(int32x4_t a, int32_t b) {
 #if defined(NEON_SIMD_INT)
   return vshrq_n_s32(a, b);
 #else
   const int32x4_t v = {{a.val[0] >> b, a.val[1] >> b, a.val[2] >> b, a.val[3] >> b}};
   return v;
 #endif
 }
 
 /** Minimum
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_min(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vmin_u32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] < b.val[0]) ? a.val[0] : b.val[0], (a.val[1] < b.val[1]) ? a.val[1] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_min(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vminq_u32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] < b.val[0]) ? a.val[0] : b.val[0], (a.val[1] < b.val[1]) ? a.val[1] : b.val[1], (a.val[2] < b.val[2]) ? a.val[2] : b.val[2], (a.val[3] < b.val[3]) ? a.val[3] : b.val[3]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_min(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vmin_s32(a, b);
 #else
   const int32x2_t v = {{(a.val[0] < b.val[0]) ? a.val[0] : b.val[0], (a.val[1] < b.val[1]) ? a.val[1] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_min(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vminq_s32(a, b);
 #else
   const int32x4_t v = {{(a.val[0] < b.val[0]) ? a.val[0] : b.val[0], (a.val[1] < b.val[1]) ? a.val[1] : b.val[1], (a.val[2] < b.val[2]) ? a.val[2] : b.val[2], (a.val[3] < b.val[3]) ? a.val[3] : b.val[3]}};
   return v;
 #endif
 }
 
 /** Maximum
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_max(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vmax_u32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] > b.val[0]) ? a.val[0] : b.val[0], (a.val[1] > b.val[1]) ? a.val[1] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_max(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vmaxq_u32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] > b.val[0]) ? a.val[0] : b.val[0], (a.val[1] > b.val[1]) ? a.val[1] : b.val[1], (a.val[2] > b.val[2]) ? a.val[2] : b.val[2], (a.val[3] > b.val[3]) ? a.val[3] : b.val[3]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_max(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vmax_s32(a, b);
 #else
   const int32x2_t v = {{(a.val[0] > b.val[0]) ? a.val[0] : b.val[0], (a.val[1] > b.val[1]) ? a.val[1] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x4_max(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vmaxq_s32(a, b);
 #else
   const int32x4_t v = {{(a.val[0] > b.val[0]) ? a.val[0] : b.val[0], (a.val[1] > b.val[1]) ? a.val[1] : b.val[1], (a.val[2] > b.val[2]) ? a.val[2] : b.val[2], (a.val[3] > b.val[3]) ? a.val[3] : b.val[3]}};
   return v;
 #endif
 }
 
 /** Pairwise Minimum
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_pmin(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vpmin_u32(a, b);
 #else
   // Note: unclear if a and b get inverted in the process, confirm.
   const uint32x2_t v = {{(a.val[0] > a.val[1]) ? a.val[0] : a.val[1], (b.val[0] > b.val[1]) ? b.val[0] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_pmin(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vpmin_s32(a, b);
 #else
   // Note: unclear if a and b get inverted in the process, confirm.
   const int32x2_t v = {{(a.val[0] > a.val[1]) ? a.val[0] : a.val[1], (b.val[0] > b.val[1]) ? b.val[0] : b.val[1]}};
   return v;
 #endif
 }
 
 /** Pairwise Maximum
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_pmax(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vpmax_u32(a, b);
 #else
   // Note: unclear if a and b get inverted in the process, confirm.
   const uint32x2_t v = {{(a.val[0] > a.val[1]) ? a.val[0] : a.val[1], (b.val[0] > b.val[1]) ? b.val[0] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x2_pmax(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vpmax_s32(a, b);
 #else
   // Note: unclear if a and b get inverted in the process, confirm.
   const int32x2_t v = {{(a.val[0] > a.val[1]) ? a.val[0] : a.val[1], (b.val[0] > b.val[1]) ? b.val[0] : b.val[1]}};
   return v;
 #endif
 }
 
 /** Less than
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_lt(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vclt_u32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] < b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] < b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_lt(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vcltq_u32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] < b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] < b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] < b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] < b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_lt(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vclt_s32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] < b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] < b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_lt(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vcltq_s32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] < b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] < b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] < b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] < b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Less than or equal
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_lte(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vcle_u32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] <= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] <= b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_lte(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vcleq_u32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] <= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] <= b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] <= b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] <= b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_lte(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vcle_s32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] <= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] <= b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_lte(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vcleq_s32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] <= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] <= b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] <= b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] <= b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Less than zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_ltz(int32x2_t a) {
 #if defined(NEON_SIMD_INT)
   //return vcltz_s32(a); // A64 only
   return vclt_s32(a, int32x2(0, 0));
   static const int32_t zero = 0;
   return vclt_s32(a, vld1_dup_s32(&zero));
 #else
   const uint32x2_t v = {{(a.val[0] < 0) ? 0xFFFFFFFF : 0x0, (a.val[1] < 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_ltz(int32x4_t a) {
 #if defined(NEON_SIMD_INT)
   // return vcltzq_s32(a); // A64 only
   static const int32_t zero = 0;
   return vcltq_s32(a, vld1q_dup_s32(&zero));
 #else
   const uint32x4_t v = {{(a.val[0] < 0) ? 0xFFFFFFFF : 0x0, (a.val[1] < 0) ? 0xFFFFFFFF : 0x0, (a.val[2] < 0) ? 0xFFFFFFFF : 0x0, (a.val[3] < 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Less than or equal zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_ltez(int32x2_t a) {
 #if defined(NEON_SIMD_INT)
   //return vclez_s32(a); // A64 only
   static const int32_t zero = 0;
   return vcle_s32(a, vld1_dup_s32(&zero));
 #else
   const uint32x2_t v = {{(a.val[0] <= 0) ? 0xFFFFFFFF : 0x0, (a.val[1] <= 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_ltez(int32x4_t a) {
 #if defined(NEON_SIMD_INT)
   //return vclezq_s32(a); // A64 only
   static const int32_t zero = 0;
   return vcleq_s32(a, vld1q_dup_s32(&zero));
 #else
   const uint32x4_t v = {{(a.val[0] <= 0) ? 0xFFFFFFFF : 0x0, (a.val[1] <= 0) ? 0xFFFFFFFF : 0x0, (a.val[2] <= 0) ? 0xFFFFFFFF : 0x0, (a.val[3] <= 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Greater than
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_gt(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vcgt_u32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] > b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] > b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_gt(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vcgtq_u32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] > b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] > b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] > b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] > b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_gt(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vcgt_s32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] > b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] > b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_gt(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vcgtq_s32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] > b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] > b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] > b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] > b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Greater than or equal
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_gte(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vcge_u32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] >= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] >= b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_gte(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vcgeq_u32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] >= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] >= b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] >= b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] >= b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_gte(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vcge_s32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] >= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] >= b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_gte(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vcgeq_s32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] >= b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] >= b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] >= b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] >= b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Greater than zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_gtz(int32x2_t a) {
 #if defined(NEON_SIMD_INT)
   //return vcgtz_s32(a); // A64 only
   static const int32_t zero = 0;
   return vcgt_s32(a, vld1_dup_s32(&zero));
 #else
   const uint32x2_t v = {{(a.val[0] > 0) ? 0xFFFFFFFF : 0x0, (a.val[1] > 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_gtz(int32x4_t a) {
 #if defined(NEON_SIMD_INT)
   //return vcgtzq_s32(a); // A64 only
   static const int32_t zero = 0;
   return vcgtq_s32(a, vld1q_dup_s32(&zero));
 #else
   const uint32x4_t v = {{(a.val[0] > 0) ? 0xFFFFFFFF : 0x0, (a.val[1] > 0) ? 0xFFFFFFFF : 0x0, (a.val[2] > 0) ? 0xFFFFFFFF : 0x0, (a.val[3] > 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Greater than or equal zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_gtez(int32x2_t a) {
 #if defined(NEON_SIMD_INT)
   //return vcgez_s32(a); // A64 only
   static const int32_t zero = 0;
   return vcge_s32(a, vld1_dup_s32(&zero));
 #else
   const uint32x2_t v = {{(a.val[0] >= 0) ? 0xFFFFFFFF : 0x0, (a.val[1] >= 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_gtez(int32x4_t a) {
 #if defined(NEON_SIMD_INT)
   //return vcgezq_s32(a); // A64 only
   static const int32_t zero = 0;
   return vcgeq_s32(a, vld1q_dup_s32(&zero));
 #else
   const uint32x4_t v = {{(a.val[0] >= 0) ? 0xFFFFFFFF : 0x0, (a.val[1] >= 0) ? 0xFFFFFFFF : 0x0, (a.val[2] >= 0) ? 0xFFFFFFFF : 0x0, (a.val[3] >= 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Equal
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_eq(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vceq_u32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] == b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] == b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_eq(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vceqq_u32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] == b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] == b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] == b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] == b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_eq(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vceq_s32(a, b);
 #else
   const uint32x2_t v = {{(a.val[0] == b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] == b.val[1]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_eq(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vceqq_s32(a, b);
 #else
   const uint32x4_t v = {{(a.val[0] == b.val[0]) ? 0xFFFFFFFF : 0x0, (a.val[1] == b.val[1]) ? 0xFFFFFFFF : 0x0, (a.val[2] == b.val[2]) ? 0xFFFFFFFF : 0x0, (a.val[3] == b.val[3]) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Equal Zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_eqz(uint32x2_t a) {
 #if defined(NEON_SIMD_INT)
   //return vceqz_u32(a); // A64 only
   static const uint32_t zero = 0;
   return vceq_u32(a, vld1_dup_u32(&zero));
 #else
   const uint32x2_t v = {{(a.val[0] == 0) ? 0xFFFFFFFF : 0x0, (a.val[1] == 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_eqz(uint32x4_t a) {
 #if defined(NEON_SIMD_INT)
   //return vceqz_u32(a); // A64 only
   static const uint32_t zero = 0;
   return vceqq_u32(a, vld1q_dup_u32(&zero));
 #else
   const uint32x4_t v = {{(a.val[0] == 0) ? 0xFFFFFFFF : 0x0, (a.val[1] == 0) ? 0xFFFFFFFF : 0x0,
                          (a.val[2] == 0) ? 0xFFFFFFFF : 0x0, (a.val[3] == 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_eqz(int32x2_t a) {
 #if defined(NEON_SIMD_INT)
   //return vceqz_s32(a); // A64 only
   static const int32_t zero = 0;
   return vceq_s32(a, vld1_dup_s32(&zero));
 #else
   const uint32x2_t v = {{(a.val[0] == 0) ? 0xFFFFFFFF : 0x0, (a.val[1] == 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_eqz(int32x4_t a) {
 #if defined(NEON_SIMD_INT)
   //return vceqz_s32(a); // A64 only
   static const int32_t zero = 0;
   return vceqq_s32(a, vld1q_dup_s32(&zero));
 #else
   const uint32x4_t v = {{(a.val[0] == 0) ? 0xFFFFFFFF : 0x0, (a.val[1] == 0) ? 0xFFFFFFFF : 0x0,
                          (a.val[2] == 0) ? 0xFFFFFFFF : 0x0, (a.val[3] == 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Bitwise and then test not zero
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_tst(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vtst_u32(a, b);
 #else
   const uint32x2_t v = {{((a.val[0] & b.val[0]) != 0) ? 0xFFFFFFFF : 0x0, ((a.val[1] & b.val[1]) != 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_tst(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vtstq_u32(a, b);
 #else
   const uint32x4_t v = {{((a.val[0] & b.val[0]) != 0) ? 0xFFFFFFFF : 0x0, ((a.val[1] & b.val[1]) != 0) ? 0xFFFFFFFF : 0x0,
                          ((a.val[2] & b.val[2]) != 0) ? 0xFFFFFFFF : 0x0, ((a.val[3] & b.val[3]) != 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 int32x2_tst(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vtst_s32(a, b);
 #else
   const uint32x2_t v = {{((a.val[0] & b.val[0]) != 0) ? 0xFFFFFFFF : 0x0, ((a.val[1] & b.val[1]) != 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 int32x4_tst(int32x4_t a, int32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vtstq_s32(a, b);
 #else
   const uint32x4_t v = {{((a.val[0] & b.val[0]) != 0) ? 0xFFFFFFFF : 0x0, ((a.val[1] & b.val[1]) != 0) ? 0xFFFFFFFF : 0x0,
                          ((a.val[2] & b.val[2]) != 0) ? 0xFFFFFFFF : 0x0, ((a.val[3] & b.val[3]) != 0) ? 0xFFFFFFFF : 0x0}};
   return v;
 #endif
 }
 
 /** Select
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_sel(uint32x2_t s, uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vbsl_u32(s, a, b);
 #else
   // Note: not completely accurate since performing select word-wise instead of bit-wise but reflects typical usage
   const uint32x2_t v = {{(s.val[0] != 0) ? a.val[0] : b.val[0], (s.val[1] != 0) ? a.val[1] : b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_sel(uint32x4_t s, uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vbslq_u32(s, a, b);
 #else
   // Note: not completely accurate since performing select word-wise instead of bit-wise but reflects typical usage
   const uint32x4_t v = {{(s.val[0] != 0) ? a.val[0] : b.val[0], (s.val[1] != 0) ? a.val[1] : b.val[1], (s.val[2] != 0) ? a.val[2] : b.val[2], (s.val[3] != 0) ? a.val[3] : b.val[3]}};
   return v;
 #endif
 }
 
 /** Bitwise OR
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_or(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vorr_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] | b.val[0], a.val[1] | b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_or(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vorrq_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] | b.val[0], a.val[1] | b.val[1], a.val[2] | b.val[2], a.val[3] | b.val[3]}};
   return v;
 #endif
 }
 
 /** Bitwise AND
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_and(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vand_u32(a, b);
 #else
   const uint32x2_t v = {{a.val[0] & b.val[0], a.val[1] & b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_and(uint32x4_t a, uint32x4_t b) {
 #if defined(NEON_SIMD_INT)
   return vandq_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0] & b.val[0], a.val[1] & b.val[1], a.val[2] & b.val[2], a.val[3] & b.val[3]}};
   return v;
 #endif
 }
 
 /** Bitwise NOT
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x2_not(uint32x2_t a) {
 #if defined(NEON_SIMD_INT)
   return vmvn_u32(a);
 #else
   const uint32x2_t v = {{~a.val[0], ~a.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x4_not(uint32x4_t a) {
 #if defined(NEON_SIMD_INT)
   return vmvnq_u32(a);
 #else
   const uint32x4_t v = {{~a.val[0], ~a.val[1], ~a.val[2], ~a.val[3]}};
   return v;
 #endif
 }
 
 /** Combine pair vectors into quad
  */
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 int32x2_comb(int32x2_t a, int32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vcombine_s32(a, b);
 #else
   const int32x4_t v = {{a.val[0], a.val[1], b.val[0], b.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 uint32x2_comb(uint32x2_t a, uint32x2_t b) {
 #if defined(NEON_SIMD_INT)
   return vcombine_u32(a, b);
 #else
   const uint32x4_t v = {{a.val[0], a.val[1], b.val[0], b.val[1]}};
   return v;
 #endif
 }
 
 /** Split
  */
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x4_low(int32x4_t a) {
 #if defined(NEON_SIMD_INT)
   return vget_low_s32(a);
 #else
   const int32x2_t v = {{a.val[0], a.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 int32x4_high(int32x4_t a) {
 #if defined(NEON_SIMD_INT)
   return vget_high_s32(a);
 #else
   const int32x2_t v = {{a.val[2], a.val[3]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x4_low(uint32x4_t a) {
 #if defined(NEON_SIMD_INT)
   return vget_low_u32(a);
 #else
   const uint32x2_t v = {{a.val[0], a.val[1]}};
   return v;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 uint32x4_high(uint32x4_t a) {
 #if defined(NEON_SIMD_INT)
   return vget_high_u32(a);
 #else
   const uint32x2_t v = {{a.val[2], a.val[3]}};
   return v;
 #endif
 }
 
  /** Clip upper bound of x to m (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 clipmaxi32x2(int32x2_t x, int32x2_t m) {
 #if defined(NEON_SIMD_INT)
   return vmin_s32(m, x);
 #else
   const int32x2_t tmp = {{(x.val[0] > m.val[0]) ? m.val[0] : x.val[0], (x.val[1] > m.val[1]) ? m.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 clipmaxi32x4(int32x4_t x, int32x4_t m) {
 #if defined(NEON_SIMD_INT)
   return vminq_s32(m, x);
 #else
   const int32x4_t tmp = {{(x.val[0] > m.val[0]) ? m.val[0] : x.val[0], (x.val[1] > m.val[1]) ? m.val[1] : x.val[1],
                             (x.val[2] > m.val[2]) ? m.val[2] : x.val[2], (x.val[3] > m.val[3]) ? m.val[3] : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip lower bound of x to m (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 clipmini32x2(int32x2_t m, int32x2_t x) {
 #if defined(NEON_SIMD_INT)
   return vmax_s32(m, x);
 #else
   const int32x2_t tmp = {{(x.val[0] < m.val[0]) ? m.val[0] : x.val[0], (x.val[1] < m.val[1]) ? m.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 clipmini32x4(int32x4_t m, int32x4_t x) {
 #if defined(NEON_SIMD_INT)
   return vmaxq_s32(m, x);
 #else
   const int32x4_t tmp = {{(x.val[0] < m.val[0]) ? m.val[0] : x.val[0], (x.val[1] < m.val[1]) ? m.val[1] : x.val[1],
                             (x.val[2] < m.val[2]) ? m.val[2] : x.val[2], (x.val[3] < m.val[3]) ? m.val[3] : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip x to min and max (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x2_t
 clipminmaxi32x2(int32x2_t min, int32x2_t x, int32x2_t max) {
 #if defined(NEON_SIMD_INT)
   return vmin_s32(max, vmax_s32(min, x));
 #else
   const int2x2_t tmp = {{(x.val[0] > max.val[0]) ? max.val[0] : (x.val[0] < min.val[0]) ? min.val[0] : x.val[0],
                          (x.val[1] > max.val[1]) ? max.val[1] : (x.val[1] < min.val[1]) ? min.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 int32x4_t
 clipminmaxi32x4(int32x4_t min, int32x4_t x, int32x4_t max) {
 #if defined(NEON_SIMD_INT)
   return vminq_s32(max, vmaxq_s32(min, x));
 #else
   const int32x4_t tmp = {{(x.val[0] > max.val[0]) ? max.val[0] : (x.val[0] < min.val[0]) ? min.val[0] : x.val[0],
                           (x.val[1] > max.val[1]) ? max.val[1] : (x.val[1] < min.val[1]) ? min.val[1] : x.val[1],
                           (x.val[2] > max.val[2]) ? max.val[2] : (x.val[2] < min.val[2]) ? min.val[2] : x.val[2],
                           (x.val[3] > max.val[3]) ? max.val[3] : (x.val[3] < min.val[3]) ? min.val[3] : x.val[3]}};
   return tmp;
 #endif
 }

 /** Clip upper bound of x to m (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 clipmaxu32x2(uint32x2_t x, uint32x2_t m) {
 #if defined(NEON_SIMD_INT)
   return vmin_u32(m, x);
 #else
   const uint32x2_t tmp = {{(x.val[0] > m.val[0]) ? m.val[0] : x.val[0], (x.val[1] > m.val[1]) ? m.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 clipmaxu32x4(uint32x4_t x, uint32x4_t m) {
 #if defined(NEON_SIMD_INT)
   return vminq_u32(m, x);
 #else
   const uint32x4_t tmp = {{(x.val[0] > m.val[0]) ? m.val[0] : x.val[0], (x.val[1] > m.val[1]) ? m.val[1] : x.val[1],
                             (x.val[2] > m.val[2]) ? m.val[2] : x.val[2], (x.val[3] > m.val[3]) ? m.val[3] : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip lower bound of x to m (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 clipminu32x2(uint32x2_t m, uint32x2_t x) {
 #if defined(NEON_SIMD_INT)
   return vmax_u32(m, x);
 #else
   const uint32x2_t tmp = {{(x.val[0] < m.val[0]) ? m.val[0] : x.val[0], (x.val[1] < m.val[1]) ? m.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 clipminu32x4(uint32x4_t m, uint32x4_t x) {
 #if defined(NEON_SIMD_INT)
   return vmaxq_u32(m, x);
 #else
   const uint32x4_t tmp = {{(x.val[0] < m.val[0]) ? m.val[0] : x.val[0], (x.val[1] < m.val[1]) ? m.val[1] : x.val[1],
                             (x.val[2] < m.val[2]) ? m.val[2] : x.val[2], (x.val[3] < m.val[3]) ? m.val[3] : x.val[3]}};
   return tmp;
 #endif
 }
 
 /** Clip x to min and max (inclusive)
  */
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x2_t
 clipminmaxu32x2(uint32x2_t min, uint32x2_t x, uint32x2_t max) {
 #if defined(NEON_SIMD_INT)
   return vmin_u32(max, vmax_u32(min, x));
 #else
   const uint32x2_t tmp = {{(x.val[0] > max.val[0]) ? max.val[0] : (x.val[0] < min.val[0]) ? min.val[0] : x.val[0],
                             (x.val[1] > max.val[1]) ? max.val[1] : (x.val[1] < min.val[1]) ? min.val[1] : x.val[1]}};
   return tmp;
 #endif
 }
 
 static inline __attribute__((optimize("Ofast"), always_inline))
 uint32x4_t
 clipminmaxu32x4(uint32x4_t min, uint32x4_t x, uint32x4_t max) {
 #if defined(NEON_SIMD_INT)
   return vminq_u32(max, vmaxq_u32(min, x));
 #else
   const uint32x4_t tmp = {{(x.val[0] > max.val[0]) ? max.val[0] : (x.val[0] < min.val[0]) ? min.val[0] : x.val[0],
                             (x.val[1] > max.val[1]) ? max.val[1] : (x.val[1] < min.val[1]) ? min.val[1] : x.val[1],
                             (x.val[2] > max.val[2]) ? max.val[2] : (x.val[2] < min.val[2]) ? min.val[2] : x.val[2],
                             (x.val[3] > max.val[3]) ? max.val[3] : (x.val[3] < min.val[3]) ? min.val[3] : x.val[3]}};
   return tmp;
 #endif
 }

 /** @} */
 
 #endif  // __mk2_int_simd_h
 