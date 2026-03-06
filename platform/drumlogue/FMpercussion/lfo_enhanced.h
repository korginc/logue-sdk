#pragma once

/**
 * @file lfo_enhanced.h
 * @brief Enhanced LFO system with bipolar modulation
 *
 * FIXED: Using central constants from constants.h
 */

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"

/**
 * Enhanced LFO state
 */
typedef struct {
    float32x4_t phase1;
    float32x4_t phase2;

    uint32_t shape_combo;
    uint32_t target1;
    uint32_t target2;
    float32x4_t depth1;
    float32x4_t depth2;
    float32x4_t rate1;
    float32x4_t rate2;

    uint8_t mod_matrix[2][2];
} lfo_enhanced_t;

/**
 * Initialize LFOs with phase independence
 */
fast_inline void lfo_enhanced_init(lfo_enhanced_t* lfo) {
    lfo->phase1 = vdupq_n_f32(0.0f);
    lfo->phase2 = vdupq_n_f32(LFO_PHASE_OFFSET);  // Using constant

    lfo->shape_combo = 0;
    lfo->target1 = LFO_TARGET_NONE;
    lfo->target2 = LFO_TARGET_NONE;
    lfo->depth1 = vdupq_n_f32(0.0f);
    lfo->depth2 = vdupq_n_f32(0.0f);
    lfo->rate1 = vdupq_n_f32(0.01f);
    lfo->rate2 = vdupq_n_f32(0.01f);

    lfo->mod_matrix[0][0] = 0;
    lfo->mod_matrix[0][1] = 0;
    lfo->mod_matrix[1][0] = 0;
    lfo->mod_matrix[1][1] = 0;
}

/**
 * Update LFO parameters
 */
fast_inline void lfo_enhanced_update(lfo_enhanced_t* lfo,
                                     uint32_t shape_combo,
                                     uint32_t target1,
                                     uint32_t target2,
                                     float depth1_percent,
                                     float depth2_percent,
                                     float rate1_percent,
                                     float rate2_percent) {

    lfo->shape_combo = shape_combo;

    // Check for circular references
    uint32_t new_target1 = target1;
    uint32_t new_target2 = target2;

    if ((new_target1 == LFO_TARGET_LFO2_PHASE &&
         new_target2 == LFO_TARGET_LFO1_PHASE) ||
        (new_target2 == LFO_TARGET_LFO1_PHASE &&
         new_target1 == LFO_TARGET_LFO2_PHASE)) {
        if (new_target1 == LFO_TARGET_LFO2_PHASE) new_target1 = LFO_TARGET_NONE;
        if (new_target2 == LFO_TARGET_LFO1_PHASE) new_target2 = LFO_TARGET_NONE;
    }

    lfo->target1 = new_target1;
    lfo->target2 = new_target2;

    lfo->mod_matrix[0][1] = (new_target1 == LFO_TARGET_LFO2_PHASE) ? 1 : 0;
    lfo->mod_matrix[1][0] = (new_target2 == LFO_TARGET_LFO1_PHASE) ? 1 : 0;

    // Map depth from -100..100 to -1.0..1.0
    float depth1 = depth1_percent / 100.0f;
    float depth2 = depth2_percent / 100.0f;

    lfo->depth1 = vdupq_n_f32(depth1);
    lfo->depth2 = vdupq_n_f32(depth2);

    // Convert rate percentage to phase increment
    float rate1 = 0.01f + (rate1_percent / 100.0f) * 0.49f;
    float rate2 = 0.01f + (rate2_percent / 100.0f) * 0.49f;

    lfo->rate1 = vdupq_n_f32(rate1);
    lfo->rate2 = vdupq_n_f32(rate2);
}

/**
 * Get LFO shape from combo
 */
fast_inline uint32_t lfo_get_shape(uint32_t combo, uint32_t lfo_num) {
    if (lfo_num == 0) {
        return combo / 3;
    } else {
        return combo % 3;
    }
}

/**
 * Generate LFO value for a specific shape
 */
fast_inline float32x4_t lfo_generate_shape(uint32_t shape, float32x4_t phase) {
    switch (shape) {
        case LFO_SHAPE_TRIANGLE: {
            float32x4_t half = vdupq_n_f32(0.5f);
            float32x4_t two = vdupq_n_f32(2.0f);
            float32x4_t diff = vsubq_f32(phase, half);
            float32x4_t abs_diff = vabsq_f32(diff);
            return vmulq_f32(abs_diff, two);
        }

        case LFO_SHAPE_RAMP: {
            return phase;
        }

        case LFO_SHAPE_CHORD: {
            // 3-step quantized: Root (0), 3rd (4 semitones), 5th (7 semitones)
            float32x4_t three = vdupq_n_f32(3.0f);
            int32x4_t step = vcvtq_s32_f32(vmulq_f32(phase, three));
            float32x4_t step_f = vcvtq_f32_s32(step);

            // Using central constants from constants.h
            float32x4_t root = vdupq_n_f32(INTERVAL_UNISON);
            float32x4_t third = vdupq_n_f32(INTERVAL_MAJOR_3RD);
            float32x4_t fifth = vdupq_n_f32(INTERVAL_PERFECT_5TH);

            return vbslq_f32(vceqq_f32(step_f, vdupq_n_f32(0.0f)), root,
                   vbslq_f32(vceqq_f32(step_f, vdupq_n_f32(1.0f)), third, fifth));
        }

        default:
            return phase;
    }
}

/**
 * Process LFOs
 */
fast_inline void lfo_enhanced_process(lfo_enhanced_t* lfo,
                                      float32x4_t* lfo1_out,
                                      float32x4_t* lfo2_out) {
    float32x4_t one = vdupq_n_f32(1.0f);

    float32x4_t phase1_inc = lfo->rate1;
    float32x4_t phase2_inc = lfo->rate2;

    if (lfo->mod_matrix[0][1]) {
        float32x4_t lfo1_val = lfo_generate_shape(
            lfo_get_shape(lfo->shape_combo, 0),
            lfo->phase1);
        phase2_inc = vaddq_f32(phase2_inc,
                               vmulq_f32(lfo1_val, lfo->depth1));
    }

    if (lfo->mod_matrix[1][0]) {
        float32x4_t lfo2_val = lfo_generate_shape(
            lfo_get_shape(lfo->shape_combo, 1),
            lfo->phase2);
        phase1_inc = vaddq_f32(phase1_inc,
                               vmulq_f32(lfo2_val, lfo->depth2));
    }

    lfo->phase1 = vaddq_f32(lfo->phase1, phase1_inc);
    lfo->phase2 = vaddq_f32(lfo->phase2, phase2_inc);

    uint32x4_t wrap1 = vcgeq_f32(lfo->phase1, one);
    uint32x4_t wrap2 = vcgeq_f32(lfo->phase2, one);

    lfo->phase1 = vbslq_f32(wrap1, vsubq_f32(lfo->phase1, one), lfo->phase1);
    lfo->phase2 = vbslq_f32(wrap2, vsubq_f32(lfo->phase2, one), lfo->phase2);

    *lfo1_out = lfo_generate_shape(lfo_get_shape(lfo->shape_combo, 0), lfo->phase1);
    *lfo2_out = lfo_generate_shape(lfo_get_shape(lfo->shape_combo, 1), lfo->phase2);
}

/**
 * Apply bipolar modulation to target
 */
fast_inline float32x4_t lfo_apply_modulation(float32x4_t target_value,
                                             float32x4_t lfo_value,
                                             float32x4_t depth,
                                             float32_t min,
                                             float32_t max) {
    float32x4_t lfo_bipolar = vsubq_f32(vmulq_f32(lfo_value, vdupq_n_f32(2.0f)),
                                        vdupq_n_f32(1.0f));

    float32x4_t modulation = vmulq_f32(lfo_bipolar, depth);
    float32x4_t result = vaddq_f32(target_value, modulation);

    result = vmaxq_f32(result, vdupq_n_f32(min));
    result = vminq_f32(result, vdupq_n_f32(max));

    return result;
}