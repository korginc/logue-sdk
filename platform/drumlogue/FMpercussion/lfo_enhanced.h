#pragma once

/**
 * @file lfo_enhanced.h
 * @brief Enhanced LFO system with bipolar modulation
 *
 * Depth range: -100 to +100 from UI maps to -1.0 to +1.0
 */

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"

/**
 * Enhanced LFO state
 */
typedef struct {
    // Primary LFOs
    float32x4_t phase1;      // LFO1 phase for all 4 voices
    float32x4_t phase2;      // LFO2 phase for all 4 voices (90° offset)

    // Parameters
    uint32_t shape_combo;     // 0-8 encoding both LFO shapes
    uint32_t target1;          // 0-5 LFO1 target
    uint32_t target2;          // 0-5 LFO2 target
    float32x4_t depth1;       // Bipolar depth (-1.0 to 1.0)
    float32x4_t depth2;       // Bipolar depth (-1.0 to 1.0)
    float32x4_t rate1;        // Rate as phase increment
    float32x4_t rate2;        // Rate as phase increment

    // Circular reference detection
    uint8_t mod_matrix[2][2];
} lfo_enhanced_t;

/**
 * Initialize LFOs with phase independence
 */
fast_inline void lfo_enhanced_init(lfo_enhanced_t* lfo) {
    lfo->phase1 = vdupq_n_f32(0.0f);
    lfo->phase2 = vdupq_n_f32(LFO_PHASE_OFFSET);

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
 * Update LFO parameters with proper bipolar depth scaling
 *
 * @param shape_combo 0-8 encoding both LFO shapes
 * @param target1 0-5 LFO1 target
 * @param target2 0-5 LFO2 target
 * @param depth1_value -100 to +100 (from UI) - maps to -1.0 to 1.0
 * @param depth2_value -100 to +100 (from UI) - maps to -1.0 to 1.0
 * @param rate1_percent 0-100 LFO1 rate
 * @param rate2_percent 0-100 LFO2 rate
 */
fast_inline void lfo_enhanced_update(lfo_enhanced_t* lfo,
                                     uint32_t shape_combo,
                                     uint32_t target1,
                                     uint32_t target2,
                                     float depth1_value,  // -100 to +100
                                     float depth2_value,  // -100 to +100
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
        // Circular reference detected!
        if (new_target1 == LFO_TARGET_LFO2_PHASE) new_target1 = LFO_TARGET_NONE;
        if (new_target2 == LFO_TARGET_LFO1_PHASE) new_target2 = LFO_TARGET_NONE;
    }

    lfo->target1 = new_target1;
    lfo->target2 = new_target2;

    // Update modulation matrix
    lfo->mod_matrix[0][1] = (new_target1 == LFO_TARGET_LFO2_PHASE) ? 1 : 0;
    lfo->mod_matrix[1][0] = (new_target2 == LFO_TARGET_LFO1_PHASE) ? 1 : 0;

    // =================================================================
    // CORRECT BIPOLAR MAPPING: -100 to +100 → -1.0 to +1.0
    // =================================================================
    // Examples:
    //   -100 → -1.0 (full negative modulation)
    //   -50  → -0.5 (half negative)
    //    0   →  0.0 (no modulation)
    //   50   →  0.5 (half positive)
    //  100   →  1.0 (full positive)
    // =================================================================
    float depth1 = depth1_value / 100.0f;
    float depth2 = depth2_value / 100.0f;

    // Clamp to ensure valid range (safety)
    if (depth1 < -1.0f) depth1 = -1.0f;
    if (depth1 > 1.0f) depth1 = 1.0f;
    if (depth2 < -1.0f) depth2 = -1.0f;
    if (depth2 > 1.0f) depth2 = 1.0f;

    lfo->depth1 = vdupq_n_f32(depth1);
    lfo->depth2 = vdupq_n_f32(depth2);

    // Convert rate percentage to phase increment (0.01 to 0.5 rad/sample)
    float rate1 = 0.01f + (rate1_percent / 100.0f) * 0.49f;
    float rate2 = 0.01f + (rate2_percent / 100.0f) * 0.49f;

    lfo->rate1 = vdupq_n_f32(rate1);
    lfo->rate2 = vdupq_n_f32(rate2);
}

/**
 * Get LFO shape for a specific LFO based on combo
 */
fast_inline uint8_t lfo_get_shape(uint8_t combo, uint8_t lfo_num) {
    // combo: 0-8 encoding both LFOs
    // lfo_num: 0 = LFO1, 1 = LFO2
    if (lfo_num == 0) {
        return combo / 3;  // Integer division gives first LFO
    } else {
        return combo % 3;  // Remainder gives second LFO
    }
}

/**
 * Generate LFO value for a specific shape
 */
fast_inline float32x4_t lfo_generate_shape(uint8_t shape, float32x4_t phase) {
    switch (shape) {
        case LFO_SHAPE_TRIANGLE: {
            // Triangle: 2 * |phase - 0.5|
            float32x4_t half = vdupq_n_f32(0.5f);
            float32x4_t two = vdupq_n_f32(2.0f);
            float32x4_t diff = vsubq_f32(phase, half);
            float32x4_t abs_diff = vabsq_f32(diff);
            return vmulq_f32(abs_diff, two);
        }

        case LFO_SHAPE_RAMP: {
            // Ramp up: phase (sawtooth)
            return phase;
        }

        case LFO_SHAPE_CHORD: {
            // 3-step quantized: Root, 3rd, 5th
            // Map phase 0-1 to 0,1,2 steps
            float32x4_t three = vdupq_n_f32(3.0f);
            int32x4_t step = vcvtq_s32_f32(vmulq_f32(phase, three));

            // Convert to float and map to intervals
            float32x4_t step_f = vcvtq_f32_s32(step);

            // Intervals: 0=1.0 (root), 1=1.26 (4 semitones), 2=1.59 (7 semitones)
            float32x4_t root = vdupq_n_f32(1.0f);
            float32x4_t third = vdupq_n_f32(1.25992f);  // 2^(4/12)
            float32x4_t fifth = vdupq_n_f32(1.49831f);  // 2^(7/12)

            return vbslq_f32(vceqq_f32(step_f, vdupq_n_f32(0.0f)), root,
                   vbslq_f32(vceqq_f32(step_f, vdupq_n_f32(1.0f)), third, fifth));
        }

        default:
            return phase;
    }
}

/**
 * Process LFOs with phase modulation support
 */
fast_inline void lfo_enhanced_process(lfo_enhanced_t* lfo,
                                      float32x4_t* lfo1_out,
                                      float32x4_t* lfo2_out) {
    float32x4_t one = vdupq_n_f32(1.0f);

    // Apply LFO-to-LFO phase modulation if enabled
    float32x4_t phase1_inc = lfo->rate1;
    float32x4_t phase2_inc = lfo->rate2;

    if (lfo->mod_matrix[0][1]) {
        // LFO1 modulates LFO2 phase
        float32x4_t lfo1_val = lfo_generate_shape(
            lfo_get_shape(lfo->shape_combo, 0),
            lfo->phase1);
        // Phase modulation: add scaled LFO1 to LFO2 phase increment
        phase2_inc = vaddq_f32(phase2_inc,
                               vmulq_f32(lfo1_val, lfo->depth1));
    }

    if (lfo->mod_matrix[1][0]) {
        // LFO2 modulates LFO1 phase
        float32x4_t lfo2_val = lfo_generate_shape(
            lfo_get_shape(lfo->shape_combo, 1),
            lfo->phase2);
        phase1_inc = vaddq_f32(phase1_inc,
                               vmulq_f32(lfo2_val, lfo->depth2));
    }

    // Update phases
    lfo->phase1 = vaddq_f32(lfo->phase1, phase1_inc);
    lfo->phase2 = vaddq_f32(lfo->phase2, phase2_inc);

    // Wrap phases to [0, 1)
    uint32x4_t wrap1 = vcgeq_f32(lfo->phase1, one);
    uint32x4_t wrap2 = vcgeq_f32(lfo->phase2, one);

    lfo->phase1 = vbslq_f32(wrap1, vsubq_f32(lfo->phase1, one), lfo->phase1);
    lfo->phase2 = vbslq_f32(wrap2, vsubq_f32(lfo->phase2, one), lfo->phase2);

    // Generate final outputs
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
    // Bipolar modulation: lfo_value ranges 0-1, depth ranges -1 to 1
    // Convert lfo from 0-1 to -1 to 1 for bipolar
    float32x4_t lfo_bipolar = vsubq_f32(vmulq_f32(lfo_value, vdupq_n_f32(2.0f)),
                                        vdupq_n_f32(1.0f));

    // Apply depth
    float32x4_t modulation = vmulq_f32(lfo_bipolar, depth);

    // Add to target and clamp
    float32x4_t result = vaddq_f32(target_value, modulation);

    // Clamp to range
    result = vmaxq_f32(result, vdupq_n_f32(min));
    result = vminq_f32(result, vdupq_n_f32(max));

    return result;
}

// ========== UNIT TEST ==========
#ifdef TEST_LFO_ENHANCED

void test_phase_independence() {
    lfo_enhanced_t lfo;
    lfo_enhanced_init(&lfo);

    // Check initial 90° offset
    float p1 = vgetq_lane_f32(lfo.phase1, 0);
    float p2 = vgetq_lane_f32(lfo.phase2, 0);

    assert(fabsf(p2 - p1 - LFO_PHASE_OFFSET) < 0.001f);
    printf("Phase independence test PASSED\n");
}

void test_bipolar_depth() {
    lfo_enhanced_t lfo;
    lfo_enhanced_init(&lfo);

    // Test depth mapping
    lfo_enhanced_update(&lfo, 0, LFO_TARGET_PITCH, LFO_TARGET_NONE,
                        0.0f, 0.0f, 50.0f, 50.0f);  // depth1 = -1.0

    float depth = vgetq_lane_f32(lfo.depth1, 0);
    assert(fabsf(depth + 1.0f) < 0.001f);  // Should be -1.0

    lfo_enhanced_update(&lfo, 0, LFO_TARGET_PITCH, LFO_TARGET_NONE,
                        100.0f, 0.0f, 50.0f, 50.0f);  // depth1 = 1.0

    depth = vgetq_lane_f32(lfo.depth1, 0);
    assert(fabsf(depth - 1.0f) < 0.001f);  // Should be 1.0

    printf("Bipolar depth test PASSED\n");
}

void test_circular_reference_prevention() {
    lfo_enhanced_t lfo;
    lfo_enhanced_init(&lfo);

    // Try to create circular reference
    lfo_enhanced_update(&lfo, 0,
                        LFO_TARGET_LFO2_PHASE,  // LFO1 -> LFO2
                        LFO_TARGET_LFO1_PHASE,  // LFO2 -> LFO1 (circular!)
                        50.0f, 50.0f, 50.0f, 50.0f);

    // Should have disabled both
    assert(lfo.target1 == LFO_TARGET_NONE);
    assert(lfo.target2 == LFO_TARGET_NONE);

    printf("Circular reference prevention test PASSED\n");
}

void test_bipolar_modulation() {
    // Test pitch modulation
    float32x4_t pitch = vdupq_n_f32(440.0f);  // A4
    float32x4_t lfo_val = vdupq_n_f32(0.5f);   // Midpoint
    float32x4_t depth = vdupq_n_f32(0.5f);     // +50% modulation

    float32x4_t result = lfo_apply_modulation(pitch, lfo_val, depth, 20.0f, 2000.0f);
    float r = vgetq_lane_f32(result, 0);

    // lfo_val 0.5 with depth 0.5 gives modulation 0.0 (since bipolar lfo = 0 at 0.5)
    assert(fabsf(r - 440.0f) < 0.1f);

    // Test with lfo at max
    lfo_val = vdupq_n_f32(1.0f);
    result = lfo_apply_modulation(pitch, lfo_val, depth, 20.0f, 2000.0f);
    r = vgetq_lane_f32(result, 0);
    assert(r > 440.0f);  // Should be higher

    printf("Bipolar modulation test PASSED\n");
}

#endif