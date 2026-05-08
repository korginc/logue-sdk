#pragma once

/**
 * @file engine_mapping.h
 * @brief Lightweight Sonaglio mapping helpers
 *
 * Current Sonaglio direction:
 * - selector-based instrument routing
 * - engines consume their two UI parameters directly
 * - global controls remain in params[] and are projected at render time
 *
 * This file is intentionally small. The older macro-target mapping structs were
 * useful during the design phase, but they are no longer part of the live path.
 */

#include <arm_neon.h>
#include <stdint.h>
#include "float_math.h"
#include "unit.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ENGINE_MAPPING_INLINE
#define ENGINE_MAPPING_INLINE static inline
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    ENGINE_KICK  = 0,
    ENGINE_SNARE = 1,
    ENGINE_METAL = 2,
    ENGINE_PERC  = 3,
    ENGINE_COUNT = 4
} engine_id_t;

/* -------------------------------------------------------------------------
 * Scalar helpers
 * ------------------------------------------------------------------------- */

ENGINE_MAPPING_INLINE float fm_clampf01(float x) {
    return (x < 0.0f) ? 0.0f : ((x > 1.0f) ? 1.0f : x);
}

ENGINE_MAPPING_INLINE float fm_clamp_bipolar(float x) {
    return (x < -1.0f) ? -1.0f : ((x > 1.0f) ? 1.0f : x);
}

ENGINE_MAPPING_INLINE float fm_norm_from_percent(int8_t v) {
    int iv = (int)v;
    if (iv < 0) iv = 0;
    if (iv > 100) iv = 100;
    return (float)iv * 0.01f;
}

ENGINE_MAPPING_INLINE float fm_bipolar_from_percent(int8_t v) {
    return fm_clamp_bipolar((float)v * 0.01f);
}

ENGINE_MAPPING_INLINE float fm_lerp(float a, float b, float t) {
    return a + (b - a) * fm_clampf01(t);
}

ENGINE_MAPPING_INLINE float fm_sq(float x) {
    return x * x;
}

ENGINE_MAPPING_INLINE float fm_pow4(float x) {
    const float x2 = x * x;
    return x2 * x2;
}

/* -------------------------------------------------------------------------
 * NEON helpers
 * ------------------------------------------------------------------------- */

ENGINE_MAPPING_INLINE float32x4_t fm_vdup01(float x) {
    return vdupq_n_f32(fm_clampf01(x));
}

ENGINE_MAPPING_INLINE float32x4_t fm_vclamp01(float32x4_t x) {
    return vminq_f32(vmaxq_f32(x, vdupq_n_f32(0.0f)), vdupq_n_f32(1.0f));
}

ENGINE_MAPPING_INLINE float32x4_t fm_vlerp(float32x4_t a,
                                           float32x4_t b,
                                           float32x4_t t) {
    t = fm_vclamp01(t);
    return vaddq_f32(a, vmulq_f32(vsubq_f32(b, a), t));
}

ENGINE_MAPPING_INLINE float32x4_t fm_vpow2(float32x4_t x) {
    return vmulq_f32(x, x);
}

ENGINE_MAPPING_INLINE float32x4_t fm_vpow4(float32x4_t x) {
    const float32x4_t x2 = vmulq_f32(x, x);
    return vmulq_f32(x2, x2);
}

/* -------------------------------------------------------------------------
 * Global envelope projections
 * ------------------------------------------------------------------------- */

/**
 * HitShape projection.
 *
 * At low values the engines receive the raw envelope.
 * At high values the envelope becomes more front-loaded and shorter.
 *
 * Peak remains close to unity, with a small boost for stronger transient
 * perception. This is safer than a simple gain multiplier because it changes
 * the contour rather than only loudness.
 */
ENGINE_MAPPING_INLINE float32x4_t fm_make_transient_env(float32x4_t envelope,
                                                        float32x4_t hit_shape) {
    const float32x4_t e  = fm_vclamp01(envelope);
    const float32x4_t hs = fm_vclamp01(hit_shape);

    const float32x4_t e2 = vmulq_f32(e, e);
    const float32x4_t e4 = vmulq_f32(e2, e2);

    // Blend from natural envelope to shorter transient envelope.
    float32x4_t shaped = fm_vlerp(e, e4, hs);

    // Modest peak compensation. Avoid too much boost because each engine
    // already has its own transient drive.
    float32x4_t gain = vaddq_f32(vdupq_n_f32(1.0f), vmulq_n_f32(hs, 0.18f));
    return vmulq_f32(shaped, gain);
}

/**
 * BodyTilt projection.
 *
 * At low values the body envelope is tighter and more percussive.
 * At high values it follows the main envelope more closely and receives a
 * small low-mid/body gain boost.
 */
ENGINE_MAPPING_INLINE float32x4_t fm_make_body_env(float32x4_t envelope,
                                                   float32x4_t body_tilt) {
    const float32x4_t e  = fm_vclamp01(envelope);
    const float32x4_t bt = fm_vclamp01(body_tilt);

    const float32x4_t e2 = vmulq_f32(e, e);

    // Low body tilt: tighter e^2 contour.
    // High body tilt: longer e contour.
    float32x4_t shaped = fm_vlerp(e2, e, bt);

    // Body compensation. Keep conservative to avoid mud in combo modes.
    float32x4_t gain = vaddq_f32(vdupq_n_f32(0.82f), vmulq_n_f32(bt, 0.28f));
    return vmulq_f32(shaped, gain);
}

/**
 * Global drive gain.
 *
 * Drive is intentionally moderate here because the revised engines already
 * have local transient saturation. This acts as a final bus push.
 */
ENGINE_MAPPING_INLINE float32x4_t fm_make_drive_gain(float32x4_t drive) {
    drive = fm_vclamp01(drive);
    return vaddq_f32(vdupq_n_f32(1.0f), vmulq_n_f32(drive, 0.75f));
}

/**
 * Fast soft clip for the output bus.
 *
 * y = x / (1 + |x|)
 *
 * Uses reciprocal approximation plus one Newton-Raphson refinement.
 */
ENGINE_MAPPING_INLINE float32x4_t fm_soft_clip(float32x4_t x) {
    const float32x4_t den = vaddq_f32(vdupq_n_f32(1.0f), vabsq_f32(x));
    float32x4_t recip = vrecpeq_f32(den);
    recip = vmulq_f32(vrecpsq_f32(den, recip), recip);
    return vmulq_f32(x, recip);
}

/**
 * Cheap cubic clip used by some engines.
 *
 * y = 1.5x - 0.5x^3, after clamping x to [-1, 1].
 */
ENGINE_MAPPING_INLINE float32x4_t fm_cubic_clip(float32x4_t x) {
    x = vmaxq_f32(vdupq_n_f32(-1.0f), vminq_f32(vdupq_n_f32(1.0f), x));
    const float32x4_t x2 = vmulq_f32(x, x);
    const float32x4_t x3 = vmulq_f32(x2, x);
    return vsubq_f32(vmulq_n_f32(x, 1.5f), vmulq_n_f32(x3, 0.5f));
}

/* -------------------------------------------------------------------------
 * Reduced selector model helpers
 * ------------------------------------------------------------------------- */

/**
 * Converts a selected engine into the Euclidean tuning lane.
 *
 * Lane convention:
 * - Kick  -> lane 0
 * - Snare -> lane 1
 * - Metal -> lane 2
 * - Perc/Tom -> lane 3
 */
ENGINE_MAPPING_INLINE int fm_engine_to_euclid_lane(engine_id_t engine) {
    switch (engine) {
        case ENGINE_KICK:  return 0;
        case ENGINE_SNARE: return 1;
        case ENGINE_METAL: return 2;
        case ENGINE_PERC:  return 3;
        default:           return 0;
    }
}

/**
 * Engine role weights for the selector-based reduced model.
 * Useful when balancing combo output later, without reintroducing per-voice
 * probability controls.
 */
ENGINE_MAPPING_INLINE float fm_engine_nominal_gain(engine_id_t engine) {
    switch (engine) {
        case ENGINE_KICK:  return 0.95f;
        case ENGINE_SNARE: return 0.80f;
        case ENGINE_METAL: return 0.70f;
        case ENGINE_PERC:  return 0.82f;
        default:           return 1.0f;
    }
}

#ifdef __cplusplus
}
#endif
