#pragma once

/**
 * @file smoothing.h
 * @brief NEON-optimized parameter smoothing to prevent zipper noise
 *
 * Linear interpolation over configurable ramp time
 * Fixed: Properly initializes all vectors, including step array
 */

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"


/**
 * Parameter smoother state for 6 pages × 4 parameters
 * All vectors are 4-lane NEON for parallel processing
 */
typedef struct {
    float32x4_t current[6];    // Current smoothed values (0-1 range)
    float32x4_t target[6];      // Target values (0-1 range)
    uint32x4_t counter[6];      // Remaining frames in ramp (0 = no ramp)
    float32x4_t step[6];        // Per-frame increment (signed)
} param_smoother_t;

/**
 * Initialize smoother with current parameter values
 * @param sm Smoother state to initialize
 * @param params Raw parameter values (0-100 range)
 *
 * FIXED: Explicitly initializes step vectors to zero
 */
fast_inline void smoother_init(param_smoother_t* sm, const uint8_t* params) {
    for (int page = 0; page < 6; page++) {
        float values[4];
        for (int i = 0; i < 4; i++) {
            // Convert from 0-100 to 0-1 range
            values[i] = params[page * 4 + i] / 100.0f;
        }

        // Initialize all vectors
        sm->current[page] = vld1q_f32(values);
        sm->target[page] = sm->current[page];     // Target starts at current
        sm->counter[page] = vdupq_n_u32(0);        // No active ramps
        sm->step[page] = vdupq_n_f32(0.0f);        // FIXED: Zero out step array
    }
}

/**
 * Set new target value with smoothing for a single parameter
 * @param sm Smoother state
 * @param page Page index (0-5)
 * @param param Parameter index within page (0-3)
 * @param new_value New raw parameter value (0-100)
 *
 * If the parameter is already ramping, this smoothly transitions
 * to the new target by recalculating the step.
 */
fast_inline void smoother_set_target(param_smoother_t* sm,
                                     uint8_t page, uint8_t param,
                                     uint8_t new_value) {
    // Safety checks
    if (page > 5 || param > 3) return;

    // Convert new value to 0-1 range
    float new_target = new_value / 100.0f;

    // Extract current target vector
    float targets[4];
    vst1q_f32(targets, sm->target[page]);

    // Check if value actually changed
    if (targets[param] == new_target) return;

    // Update target
    targets[param] = new_target;
    sm->target[page] = vld1q_f32(targets);

    // Get current smoothed values
    float current[4];
    vst1q_f32(current, sm->current[page]);

    // Calculate step for this parameter
    float diff = new_target - current[param];
    float step = diff / SMOOTH_FRAMES;

    // Update step vector (only changing one lane)
    float steps[4];
    vst1q_f32(steps, sm->step[page]);
    steps[param] = step;
    sm->step[page] = vld1q_f32(steps);

    // Set counter for this parameter
    uint32_t counters[4];
    vst1q_u32(counters, sm->counter[page]);
    counters[param] = SMOOTH_FRAMES;
    sm->counter[page] = vld1q_u32(counters);
}

/**
 * Process one sample of smoothing for all parameters
 * @param sm Smoother state
 * @param smoothed Output smoothed vectors for all 6 pages
 *
 * This should be called exactly once per sample before using parameters
 */
fast_inline void smoother_process(param_smoother_t* sm,
                                  float32x4_t smoothed[6]) {
    for (int page = 0; page < 6; page++) {
        // =================================================================
        // 1. Apply step to current values for all parameters
        // =================================================================
        sm->current[page] = vaddq_f32(sm->current[page], sm->step[page]);

        // =================================================================
        // 2. Decrement counters for all parameters
        // =================================================================
        // Subtract 1 from all counters, but clamp to 0 using max
        uint32x4_t one = vdupq_n_u32(1);
        uint32x4_t new_counters = vsubq_u32(sm->counter[page], one);

        // If subtraction underflowed (counter was 0), set back to 0
        uint32x4_t underflow = vcgtq_u32(one, sm->counter[page]);  // 1 > counter
        sm->counter[page] = vbslq_u32(underflow,
                                      vdupq_n_u32(0),
                                      new_counters);

        // =================================================================
        // 3. Check which parameters have finished ramping
        // =================================================================
        uint32x4_t done = vceqq_u32(sm->counter[page], vdupq_n_u32(0));

        // =================================================================
        // 4. Snap finished parameters to target and zero their step
        // =================================================================
        sm->current[page] = vbslq_f32(vreinterpretq_f32_u32(done),
                                      sm->target[page],
                                      sm->current[page]);

        // Zero out step for finished parameters
        sm->step[page] = vbslq_f32(vreinterpretq_f32_u32(done),
                                   vdupq_n_f32(0.0f),
                                   sm->step[page]);

        // =================================================================
        // 5. Clamp current values to valid range [0, 1]
        // =================================================================
        sm->current[page] = vmaxq_f32(vdupq_n_f32(0.0f),
                                       vminq_f32(vdupq_n_f32(1.0f),
                                                sm->current[page]));

        // Provide smoothed output
        smoothed[page] = sm->current[page];
    }
}

/**
 * Check if any parameters are still smoothing
 * @return 1 if smoothing active, 0 if all parameters settled
 */
fast_inline uint32_t smoother_active(const param_smoother_t* sm) {
    uint32_t active = 0;

    for (int page = 0; page < 6; page++) {
        // Check if any counter > 0
        uint32x4_t counters = sm->counter[page];
        uint32_t page_active = vgetq_lane_u32(counters, 0) |
                               vgetq_lane_u32(counters, 1) |
                               vgetq_lane_u32(counters, 2) |
                               vgetq_lane_u32(counters, 3);
        active |= page_active;
    }

    return active;
}

/**
 * Get current smoothed value for a specific parameter
 * @param sm Smoother state
 * @param page Page index (0-5)
 * @param param Parameter index (0-3)
 * @return Smoothed value in 0-1 range
 */
fast_inline float smoother_get(const param_smoother_t* sm,
                               uint8_t page, uint8_t param) {
    if (page > 5 || param > 3) return 0.0f;

    float vals[4];
    vst1q_f32(vals, sm->current[page]);
    return vals[param];
}

/**
 * Get current smoothed value scaled to parameter range
 * @param sm Smoother state
 * @param page Page index (0-5)
 * @param param Parameter index (0-3)
 * @param min Minimum output value
 * @param max Maximum output value
 * @return Smoothed value scaled to [min, max]
 */
fast_inline float smoother_get_scaled(const param_smoother_t* sm,
                                      uint8_t page, uint8_t param,
                                      float min, float max) {
    float norm = smoother_get(sm, page, param);
    return min + norm * (max - min);
}

/**
 * Get current smoothed value for bipolar parameters (-1 to 1)
 * @param sm Smoother state
 * @param page Page index (0-5)
 * @param param Parameter index (0-3)
 * @return Smoothed value in -1 to 1 range
 */
fast_inline float smoother_get_bipolar(const param_smoother_t* sm,
                                       uint8_t page, uint8_t param) {
    float norm = smoother_get(sm, page, param);
    return (norm * 2.0f) - 1.0f;  // 0-1 → -1 to 1
}

// ========== UNIT TEST ==========
#ifdef TEST_SMOOTHING

#include <stdio.h>
#include <assert.h>
#include <math.h>

void test_smoothing_initialization() {
    param_smoother_t sm;
    uint8_t params[24];

    // Initialize with test values
    for (int i = 0; i < 24; i++) {
        params[i] = i * 4;  // 0, 4, 8, 12, ...
    }

    smoother_init(&sm, params);

    // Verify initialization
    for (int page = 0; page < 6; page++) {
        float current[4];
        vst1q_f32(current, sm.current[page]);

        for (int param = 0; param < 4; param++) {
            float expected = params[page * 4 + param] / 100.0f;
            assert(fabsf(current[param] - expected) < 0.001f);
        }

        // Verify step initialized to zero
        float step[4];
        vst1q_f32(step, sm.step[page]);
        for (int param = 0; param < 4; param++) {
            assert(step[param] == 0.0f);
        }
    }

    printf("Smoother initialization test PASSED\n");
}

void test_smoothing_ramp() {
    param_smoother_t sm;
    uint8_t params[24] = {0};  // All zeros

    smoother_init(&sm, params);

    // Set page 0, param 0 to target 100
    smoother_set_target(&sm, 0, 0, 100);

    float last_value = 0.0f;
    float smoothed[6];

    // Run through ramp
    for (int i = 0; i < SMOOTH_FRAMES + 10; i++) {
        float32x4_t smoothed_vec[6];
        smoother_process(&sm, smoothed_vec);

        // Extract page 0, param 0
        float vals[4];
        vst1q_f32(vals, smoothed_vec[0]);
        float current = vals[0];

        if (i < SMOOTH_FRAMES) {
            // Should be ramping up linearly
            assert(current > last_value);
            assert(current < 1.0f);

            // Check linearity (approx constant step)
            if (i > 0) {
                float step = current - last_value;
                float expected_step = 1.0f / SMOOTH_FRAMES;
                assert(fabsf(step - expected_step) < 0.001f);
            }
        } else {
            // Should be at target
            assert(fabsf(current - 1.0f) < 0.001f);
        }

        last_value = current;
    }

    // Verify smoother reports done
    assert(smoother_active(&sm) == 0);

    printf("Smoothing ramp test PASSED\n");
}

void test_multiple_parameters() {
    param_smoother_t sm;
    uint8_t params[24] = {0};

    smoother_init(&sm, params);

    // Set different targets for different parameters
    smoother_set_target(&sm, 0, 0, 100);  // Page0, param0: 0→100
    smoother_set_target(&sm, 0, 1, 0);    // Page0, param1: 0→0 (no change)
    smoother_set_target(&sm, 1, 2, 50);   // Page1, param2: 0→50

    // Process and verify independent smoothing
    for (int i = 0; i < SMOOTH_FRAMES; i++) {
        float32x4_t smoothed_vec[6];
        smoother_process(&sm, smoothed_vec);

        // Check page0 values
        float page0_vals[4];
        vst1q_f32(page0_vals, smoothed_vec[0]);

        if (i < SMOOTH_FRAMES - 1) {
            // Param0 should be increasing
            assert(page0_vals[0] > 0.0f);
            // Param1 should stay at 0 (no step)
            assert(page0_vals[1] == 0.0f);
        }

        // Check page1 values
        float page1_vals[4];
        vst1q_f32(page1_vals, smoothed_vec[1]);
        assert(page1_vals[2] > 0.0f);  // Param2 increasing
        assert(page1_vals[0] == 0.0f); // Others unchanged
    }

    printf("Multiple parameter smoothing test PASSED\n");
}

void test_bipolar_conversion() {
    param_smoother_t sm;
    uint8_t params[24];

    // Set param to 0 (should map to -1.0 bipolar)
    params[0] = 0;
    smoother_init(&sm, params);

    float bipolar = smoother_get_bipolar(&sm, 0, 0);
    assert(fabsf(bipolar + 1.0f) < 0.001f);

    // Set param to 50 (should map to 0.0 bipolar)
    params[0] = 50;
    smoother_init(&sm, params);
    bipolar = smoother_get_bipolar(&sm, 0, 0);
    assert(fabsf(bipolar) < 0.001f);

    // Set param to 100 (should map to 1.0 bipolar)
    params[0] = 100;
    smoother_init(&sm, params);
    bipolar = smoother_get_bipolar(&sm, 0, 0);
    assert(fabsf(bipolar - 1.0f) < 0.001f);

    printf("Bipolar conversion test PASSED\n");
}

int main() {
    test_smoothing_initialization();
    test_smoothing_ramp();
    test_multiple_parameters();
    test_bipolar_conversion();
    return 0;
}

#endif // TEST_SMOOTHING