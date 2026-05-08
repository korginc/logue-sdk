#pragma once

/**
 * @file lfo_enhanced.h
 * @brief Lightweight dual-LFO system for Sonaglio
 *
 * Current Sonaglio role:
 * - two simple modulation sources
 * - unipolar shape generation in the range 0..1
 * - bipolar depth stored as -1..1
 * - safe phase wrapping for positive and negative modulation
 *
 * Notes:
 * - The active synth path decides how each target interprets the LFO value.
 * - Do not put engine-specific behavior here.
 * - LFO-to-LFO phase/rate modulation is kept but heavily scaled for safety.
 */

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"

#ifndef LFO_ENHANCED_PHASE_MOD_SCALE
// Phase-mod targets should never add raw ±1 cycle/sample.
// This value is in cycles/sample at full depth and full bipolar LFO value.
// 0.00035 ~= 16.8 Hz equivalent modulation at 48 kHz.
#define LFO_ENHANCED_PHASE_MOD_SCALE 0.00035f
#endif

#ifndef LFO_ENHANCED_RATE_MIN_HZ
#define LFO_ENHANCED_RATE_MIN_HZ 0.05f
#endif

#ifndef LFO_ENHANCED_RATE_MAX_HZ
#define LFO_ENHANCED_RATE_MAX_HZ 18.0f
#endif

#ifndef LFO_ENHANCED_SAMPLE_RATE
#define LFO_ENHANCED_SAMPLE_RATE SAMPLE_RATE
#endif

/**
 * Enhanced LFO state.
 *
 * phase/rate/depth are NEON vectors because the previous architecture used one
 * value per lane. In the reduced selector design all lanes often carry the same
 * value, but keeping the vector form avoids touching the rest of the process
 * layer and remains cheap on ARMv7 NEON.
 */
typedef struct {
    float32x4_t phase1;
    float32x4_t phase2;

    uint32_t shape_combo;   // 0..8, shape1 = combo / 3, shape2 = combo % 3
    uint32_t target1;
    uint32_t target2;

    float32x4_t depth1;     // -1..1
    float32x4_t depth2;     // -1..1
    float32x4_t rate1;      // cycles per sample
    float32x4_t rate2;      // cycles per sample

    uint8_t mod_matrix[2][2];
} lfo_enhanced_t;

/* -------------------------------------------------------------------------
 * Small helpers
 * ------------------------------------------------------------------------- */

fast_inline float lfo_clampf(float x, float lo, float hi) {
    return (x < lo) ? lo : ((x > hi) ? hi : x);
}

fast_inline uint32_t lfo_clamp_u32(uint32_t x, uint32_t hi) {
    return (x > hi) ? hi : x;
}

/**
 * Wrap phase to [0, 1).
 *
 * This handles both positive overflow and negative phase. The old code only
 * handled phase >= 1, which becomes unsafe if phase modulation can go negative.
 */
fast_inline float32x4_t lfo_wrap_phase(float32x4_t phase) {
    const float32x4_t one = vdupq_n_f32(1.0f);
    const float32x4_t zero = vdupq_n_f32(0.0f);

    uint32x4_t ge_one = vcgeq_f32(phase, one);
    phase = vbslq_f32(ge_one, vsubq_f32(phase, one), phase);

    uint32x4_t lt_zero = vcltq_f32(phase, zero);
    phase = vbslq_f32(lt_zero, vaddq_f32(phase, one), phase);

    // One more positive wrap for safety when a high rate jumps more than one
    // cycle. Normal configured rates should not require this.
    ge_one = vcgeq_f32(phase, one);
    phase = vbslq_f32(ge_one, vsubq_f32(phase, one), phase);

    return phase;
}

/**
 * Convert unipolar 0..1 LFO value to bipolar -1..1.
 */
fast_inline float32x4_t lfo_unipolar_to_bipolar(float32x4_t x) {
    return vsubq_f32(vmulq_n_f32(x, 2.0f), vdupq_n_f32(1.0f));
}

/**
 * Clamp vector to [lo, hi].
 */
fast_inline float32x4_t lfo_vclamp(float32x4_t x, float lo, float hi) {
    return vmaxq_f32(vdupq_n_f32(lo), vminq_f32(vdupq_n_f32(hi), x));
}

/**
 * Initialize LFOs with phase independence.
 */
fast_inline void lfo_enhanced_init(lfo_enhanced_t* lfo) {
    lfo->phase1 = vdupq_n_f32(0.0f);
    lfo->phase2 = vdupq_n_f32(LFO_PHASE_OFFSET);

    lfo->shape_combo = 0;
    lfo->target1 = LFO_TARGET_NONE;
    lfo->target2 = LFO_TARGET_NONE;

    lfo->depth1 = vdupq_n_f32(0.0f);
    lfo->depth2 = vdupq_n_f32(0.0f);

    const float default_rate = 1.0f / (float)LFO_ENHANCED_SAMPLE_RATE;
    lfo->rate1 = vdupq_n_f32(default_rate);
    lfo->rate2 = vdupq_n_f32(default_rate);

    lfo->mod_matrix[0][0] = 0;
    lfo->mod_matrix[0][1] = 0;
    lfo->mod_matrix[1][0] = 0;
    lfo->mod_matrix[1][1] = 0;
}

/**
 * Update LFO parameters.
 *
 * depth values are expected as -100..+100 from UI/smoother.
 * rate values are expected as 0..100.
 */
fast_inline void lfo_enhanced_update(lfo_enhanced_t* lfo,
                                     int32_t shape_combo,
                                     uint32_t target1,
                                     uint32_t target2,
                                     float depth1_value,
                                     float depth2_value,
                                     float rate1_percent,
                                     float rate2_percent) {
    if (shape_combo < 0) shape_combo = 0;
    if (shape_combo > 8) shape_combo = 8;
    lfo->shape_combo = (uint32_t)shape_combo;

    uint32_t new_target1 = target1;
    uint32_t new_target2 = target2;

    // Keep the previous circular reference guard, but make the outcome
    // deterministic: if both LFOs try to phase-mod each other, disable LFO2's
    // phase target and keep LFO1's assignment.
    if (new_target1 == LFO_TARGET_LFO2_PHASE &&
        new_target2 == LFO_TARGET_LFO1_PHASE) {
        new_target2 = LFO_TARGET_NONE;
    }

    lfo->target1 = new_target1;
    lfo->target2 = new_target2;

    lfo->mod_matrix[0][0] = 0;
    lfo->mod_matrix[0][1] = (new_target1 == LFO_TARGET_LFO2_PHASE) ? 1 : 0;
    lfo->mod_matrix[1][0] = (new_target2 == LFO_TARGET_LFO1_PHASE) ? 1 : 0;
    lfo->mod_matrix[1][1] = 0;

    const float d1 = lfo_clampf(depth1_value * 0.01f, -1.0f, 1.0f);
    const float d2 = lfo_clampf(depth2_value * 0.01f, -1.0f, 1.0f);

    lfo->depth1 = vdupq_n_f32(d1);
    lfo->depth2 = vdupq_n_f32(d2);

    rate1_percent = lfo_clampf(rate1_percent, 0.0f, 100.0f);
    rate2_percent = lfo_clampf(rate2_percent, 0.0f, 100.0f);

    // Slightly conservative max rate for percussion macros. Very high LFO
    // rates tend to become audio-rate AM/FM side effects rather than useful
    // performance modulation.
    const float rate_span = LFO_ENHANCED_RATE_MAX_HZ - LFO_ENHANCED_RATE_MIN_HZ;
    const float rate1_hz = LFO_ENHANCED_RATE_MIN_HZ + (rate1_percent * 0.01f) * rate_span;
    const float rate2_hz = LFO_ENHANCED_RATE_MIN_HZ + (rate2_percent * 0.01f) * rate_span;

    lfo->rate1 = vdupq_n_f32(rate1_hz / (float)LFO_ENHANCED_SAMPLE_RATE);
    lfo->rate2 = vdupq_n_f32(rate2_hz / (float)LFO_ENHANCED_SAMPLE_RATE);
}

/** Important note - TODO

This file still keeps the existing shape model:

Triangle
Ramp
Chord

I did not add new shapes yet. For the current reduced selector-based Sonaglio, I would keep it stable and only add new LFO shapes later if presets clearly need them.
 */


 /**
 * Get LFO shape from combo.
 */
fast_inline uint32_t lfo_get_shape(uint32_t combo, uint32_t lfo_num) {
    combo = lfo_clamp_u32(combo, 8);
    return (lfo_num == 0) ? (combo / 3U) : (combo % 3U);
}

/**
 * Generate unipolar LFO value in the range 0..1.
 */
fast_inline float32x4_t lfo_generate_shape(uint32_t shape, float32x4_t phase) {
    phase = lfo_wrap_phase(phase);

    switch (shape) {
        case LFO_SHAPE_TRIANGLE: {
            // Standard unipolar triangle:
            // phase 0 -> 0, 0.5 -> 1, 1 -> 0.
            const float32x4_t one = vdupq_n_f32(1.0f);
            float32x4_t tri = vsubq_f32(one,
                            vabsq_f32(vsubq_f32(vmulq_n_f32(phase, 2.0f),
                                                one)));
            return lfo_vclamp(tri, 0.0f, 1.0f);
        }

        case LFO_SHAPE_RAMP:
            return phase;

        case LFO_SHAPE_CHORD: {
            // Root -> major 3rd -> perfect 5th.
            // Output is intentionally small and unipolar because process code
            // can use it directly for pitch/chord targets.
            float32x4_t step_pos = vmulq_n_f32(phase, 3.0f);
            int32x4_t step = vcvtq_s32_f32(step_pos);
            step = vmaxq_s32(vminq_s32(step, vdupq_n_s32(2)), vdupq_n_s32(0));

            const uint32x4_t is0 = vceqq_s32(step, vdupq_n_s32(0));
            const uint32x4_t is1 = vceqq_s32(step, vdupq_n_s32(1));

            const float32x4_t root  = vdupq_n_f32(0.0f);
            const float32x4_t third = vdupq_n_f32(4.0f / 24.0f);
            const float32x4_t fifth = vdupq_n_f32(7.0f / 24.0f);

            return vbslq_f32(is0, root,
                   vbslq_f32(is1, third, fifth));
        }

        default:
            return phase;
    }
}

/**
 * Process LFOs.
 *
 * Output values are unipolar 0..1. Target application can convert to bipolar
 * where appropriate with lfo_unipolar_to_bipolar().
 */
fast_inline void lfo_enhanced_process(lfo_enhanced_t* lfo,
                                      float32x4_t* lfo1_out,
                                      float32x4_t* lfo2_out) {
    float32x4_t phase1_inc = lfo->rate1;
    float32x4_t phase2_inc = lfo->rate2;

    if (lfo->mod_matrix[0][1]) {
        float32x4_t lfo1_val = lfo_generate_shape(lfo_get_shape(lfo->shape_combo, 0),
                                                  lfo->phase1);
        float32x4_t lfo1_bipolar = lfo_unipolar_to_bipolar(lfo1_val);
        phase2_inc = vaddq_f32(phase2_inc,
                               vmulq_f32(vmulq_n_f32(lfo1_bipolar,
                                                     LFO_ENHANCED_PHASE_MOD_SCALE),
                                          lfo->depth1));
    }

    if (lfo->mod_matrix[1][0]) {
        float32x4_t lfo2_val = lfo_generate_shape(lfo_get_shape(lfo->shape_combo, 1),
                                                  lfo->phase2);
        float32x4_t lfo2_bipolar = lfo_unipolar_to_bipolar(lfo2_val);
        phase1_inc = vaddq_f32(phase1_inc,
                               vmulq_f32(vmulq_n_f32(lfo2_bipolar,
                                                     LFO_ENHANCED_PHASE_MOD_SCALE),
                                          lfo->depth2));
    }

    lfo->phase1 = lfo_wrap_phase(vaddq_f32(lfo->phase1, phase1_inc));
    lfo->phase2 = lfo_wrap_phase(vaddq_f32(lfo->phase2, phase2_inc));

    *lfo1_out = lfo_generate_shape(lfo_get_shape(lfo->shape_combo, 0), lfo->phase1);
    *lfo2_out = lfo_generate_shape(lfo_get_shape(lfo->shape_combo, 1), lfo->phase2);
}

/**
 * Apply bipolar modulation to a target value.
 *
 * The LFO input is expected to be unipolar 0..1.
 * depth is expected to be -1..1.
 */
fast_inline float32x4_t lfo_apply_modulation(float32x4_t target_value,
                                             float32x4_t lfo_value,
                                             float32x4_t depth,
                                             float32_t min,
                                             float32_t max) {
    float32x4_t bipolar = lfo_unipolar_to_bipolar(lfo_value);
    float32x4_t result = vaddq_f32(target_value, vmulq_f32(bipolar, depth));
    return vmaxq_f32(vdupq_n_f32(min), vminq_f32(vdupq_n_f32(max), result));
}

/**
 * Apply scaled bipolar modulation.
 *
 * Useful for targets where full depth should mean "some musically safe amount"
 * rather than raw ±1.0.
 */
fast_inline float32x4_t lfo_apply_modulation_scaled(float32x4_t target_value,
                                                    float32x4_t lfo_value,
                                                    float32x4_t depth,
                                                    float amount,
                                                    float32_t min,
                                                    float32_t max) {
    float32x4_t bipolar = lfo_unipolar_to_bipolar(lfo_value);
    float32x4_t result = vaddq_f32(target_value,
                                   vmulq_f32(vmulq_n_f32(bipolar, amount), depth));
    return vmaxq_f32(vdupq_n_f32(min), vminq_f32(vdupq_n_f32(max), result));
}
