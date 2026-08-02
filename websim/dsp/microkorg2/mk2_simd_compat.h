// websim SIMD compatibility shim for the microKORG2 "real" units.
//
// KORG's SIMD utility headers (common/utils/{int,float,fixed}_simd.h) have two
// code paths gated on __ARM_NEON: a native NEON path (#include <arm_neon.h> +
// intrinsics) and a portable "struct" path (struct { T val[N]; } with .val[]
// access). The websim wasm build stays entirely on the struct path — it is pure
// scalar C++ and needs no wasm SIMD (-msimd128). That keeps KORG's vectorized
// code compiling, but leaves two gaps:
//
//   1. A handful of units (currently only `vox`) call a few NEON intrinsics
//      *directly* in their DSP. On the struct path those are undeclared.
//   2. Some unit/common headers include <arm_neon.h> unconditionally (e.g.
//      MorphEQ.h, and common/attributes.h transitively), which pulls in
//      Emscripten's SIMDe and defines float32x4_t et al. as *vector* types that
//      collide with KORG's struct typedefs.
//
// This header (force-included via emcc -include, ahead of the unit's own
// includes) closes gap 1: it materialises the struct-shape SIMD types, then
// defines the directly-used intrinsics as scalar .val[] operations. Gap 2 is
// closed by the sibling websim/dsp/microkorg2/arm_neon.h, which shadows SIMDe.
//
// See docs/plans/WEBSIM_FOLLOWUP_PLAN.md §A.
#ifndef WEBSIM_MK2_SIMD_COMPAT_H_
#define WEBSIM_MK2_SIMD_COMPAT_H_

// Pull KORG's struct-shape SIMD types. float_simd.h includes int_simd.h, which
// together define float32x{2,4}_t / int32x{2,4}_t / uint32x{2,4}_t as structs.
// Do NOT include fixed_simd.h here: it depends on fixed_math.h (q31_to_f32 et
// al.), which the unit includes later — pulling it early breaks that ordering.
#include "utils/float_simd.h"

// --- reciprocal estimate -----------------------------------------------------
// Hardware vrecpe* is an ~8-bit estimate; an exact reciprocal is fine (more
// accurate) for the sim, which is tolerance-based rather than bit-exact.
static inline float32x4_t vrecpeq_f32(float32x4_t a) {
  float32x4_t r;
  for (int i = 0; i < 4; ++i) r.val[i] = 1.0f / a.val[i];
  return r;
}
static inline float32x2_t vrecpe_f32(float32x2_t a) {
  float32x2_t r;
  for (int i = 0; i < 2; ++i) r.val[i] = 1.0f / a.val[i];
  return r;
}

// --- bitwise xor -------------------------------------------------------------
static inline int32x4_t veorq_s32(int32x4_t a, int32x4_t b) {
  int32x4_t r;
  for (int i = 0; i < 4; ++i) r.val[i] = a.val[i] ^ b.val[i];
  return r;
}
static inline int32x2_t veor_s32(int32x2_t a, int32x2_t b) {
  int32x2_t r;
  for (int i = 0; i < 2; ++i) r.val[i] = a.val[i] ^ b.val[i];
  return r;
}

// --- bitwise select: per lane (mask & a) | (~mask & b) -----------------------
static inline int32x4_t vbslq_s32(uint32x4_t mask, int32x4_t a, int32x4_t b) {
  int32x4_t r;
  for (int i = 0; i < 4; ++i)
    r.val[i] = (int32_t)((mask.val[i] & (uint32_t)a.val[i]) |
                         (~mask.val[i] & (uint32_t)b.val[i]));
  return r;
}

#endif  // WEBSIM_MK2_SIMD_COMPAT_H_
