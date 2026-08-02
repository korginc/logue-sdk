// websim shadow for <arm_neon.h> on the microKORG2 build.
//
// Some microKORG2 unit/common headers include <arm_neon.h> unconditionally
// (MorphEQ.h directly; common/attributes.h and common/dsp/LinearSmoother.h
// transitively for the FX units). On the wasm target that resolves to
// Emscripten's SIMDe, whose vector typedefs (simde_float32x4_t, …) collide with
// KORG's struct-shape SIMD types used on the non-NEON path.
//
// Placing this file on the include path ahead of the sysroot (via
// `-I websim/dsp/microkorg2`) shadows SIMDe so those includes become harmless.
// We stay entirely on KORG's struct representation; the few NEON intrinsics that
// units call directly are provided by mk2_simd_compat.h.
//
// See docs/plans/WEBSIM_FOLLOWUP_PLAN.md §A.
#ifndef WEBSIM_MK2_ARM_NEON_SHADOW_H_
#define WEBSIM_MK2_ARM_NEON_SHADOW_H_
#include "mk2_simd_compat.h"
#endif  // WEBSIM_MK2_ARM_NEON_SHADOW_H_
