#pragma once

/**
 * @file smoothing.h
 * @brief NEON-optimized generic parameter smoothing
 *
 * This is a generic 24-parameter smoother:
 *   6 pages × 4 parameters = 24 controls
 *
 * Current Sonaglio note:
 * - LFO-specific smoothing is handled in lfo_smoothing.h.
 * - Engine parameters are often updated directly into engine-derived state.
 * - This generic smoother is still useful for tests/tools or future global
 *   parameter smoothing, but it should not duplicate the dedicated LFO smoother.
 */

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"

#ifndef PARAM_SMOOTHER_PAGES
#define PARAM_SMOOTHER_PAGES 6
#endif

#ifndef PARAM_SMOOTHER_LANES
#define PARAM_SMOOTHER_LANES 4
#endif

#ifndef PARAM_SMOOTHER_TOTAL
#define PARAM_SMOOTHER_TOTAL (PARAM_SMOOTHER_PAGES * PARAM_SMOOTHER_LANES)
#endif

#ifndef SMOOTH_FRAMES
#define SMOOTH_FRAMES 48
#endif

/**
 * Parameter smoother state for 6 pages × 4 parameters.
 * All vectors are 4-lane NEON for parallel processing.
 */
typedef struct {
    float32x4_t current[PARAM_SMOOTHER_PAGES]; // Current smoothed values, normalized 0..1
    float32x4_t target[PARAM_SMOOTHER_PAGES];  // Target values, normalized 0..1
    uint32x4_t  counter[PARAM_SMOOTHER_PAGES]; // Remaining samples in ramp
    float32x4_t step[PARAM_SMOOTHER_PAGES];    // Per-sample increment
} param_smoother_t;

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

fast_inline float smoother_clamp01(float x) {
    return (x < 0.0f) ? 0.0f : ((x > 1.0f) ? 1.0f : x);
}

fast_inline uint8_t smoother_clamp_percent_i32(int32_t v) {
    if (v < 0) return 0;
    if (v > 100) return 100;
    return (uint8_t)v;
}

fast_inline float smoother_norm_from_i32(int32_t v) {
    return (float)smoother_clamp_percent_i32(v) * 0.01f;
}

fast_inline uint8_t smoother_page_from_index(uint8_t index) {
    return index >> 2; // / 4
}

fast_inline uint8_t smoother_lane_from_index(uint8_t index) {
    return index & 3U; // % 4
}

fast_inline uint32_t smoother_index_valid(uint8_t index) {
    return index < PARAM_SMOOTHER_TOTAL;
}

/* -------------------------------------------------------------------------
 * Init
 * ------------------------------------------------------------------------- */

/**
 * Initialize smoother with raw parameter values.
 *
 * Params may be int8_t or uint8_t in the wider codebase. This initializer takes
 * int8_t so negative values are clamped safely instead of being reinterpreted
 * as large unsigned values.
 */
fast_inline void smoother_init(param_smoother_t* sm, const int8_t* params) {
    for (int page = 0; page < PARAM_SMOOTHER_PAGES; ++page) {
        float values[PARAM_SMOOTHER_LANES];

        for (int lane = 0; lane < PARAM_SMOOTHER_LANES; ++lane) {
            values[lane] = smoother_norm_from_i32((int32_t)params[page * PARAM_SMOOTHER_LANES + lane]);
        }

        sm->current[page] = vld1q_f32(values);
        sm->target[page]  = sm->current[page];
        sm->counter[page] = vdupq_n_u32(0);
        sm->step[page]    = vdupq_n_f32(0.0f);
    }
}

/**
 * Unsigned compatibility initializer.
 */
fast_inline void smoother_init_u8(param_smoother_t* sm, const uint8_t* params) {
    for (int page = 0; page < PARAM_SMOOTHER_PAGES; ++page) {
        float values[PARAM_SMOOTHER_LANES];

        for (int lane = 0; lane < PARAM_SMOOTHER_LANES; ++lane) {
            values[lane] = smoother_norm_from_i32((int32_t)params[page * PARAM_SMOOTHER_LANES + lane]);
        }

        sm->current[page] = vld1q_f32(values);
        sm->target[page]  = sm->current[page];
        sm->counter[page] = vdupq_n_u32(0);
        sm->step[page]    = vdupq_n_f32(0.0f);
    }
}

/* -------------------------------------------------------------------------
 * Setters
 * ------------------------------------------------------------------------- */

/**
 * Set a target by page/lane.
 *
 * new_value is a raw percent value, normally 0..100. Out-of-range values are
 * clamped. Negative int8_t values therefore become 0, which is correct for this
 * generic unipolar smoother.
 */
fast_inline void smoother_set_target(param_smoother_t* sm,
                                     uint8_t page,
                                     uint8_t param,
                                     int32_t new_value) {
    if (page >= PARAM_SMOOTHER_PAGES || param >= PARAM_SMOOTHER_LANES) return;

    const float new_target = smoother_norm_from_i32(new_value);

    float targets[PARAM_SMOOTHER_LANES];
    float currents[PARAM_SMOOTHER_LANES];
    float steps[PARAM_SMOOTHER_LANES];
    uint32_t counters[PARAM_SMOOTHER_LANES];

    vst1q_f32(targets, sm->target[page]);
    vst1q_f32(currents, sm->current[page]);
    vst1q_f32(steps, sm->step[page]);
    vst1q_u32(counters, sm->counter[page]);

    if (targets[param] == new_target) return;

    targets[param] = new_target;
    steps[param] = (new_target - currents[param]) * (1.0f / (float)SMOOTH_FRAMES);
    counters[param] = SMOOTH_FRAMES;

    sm->target[page]  = vld1q_f32(targets);
    sm->step[page]    = vld1q_f32(steps);
    sm->counter[page] = vld1q_u32(counters);
}

/**
 * Set a target by flat parameter index, 0..23.
 */
fast_inline void smoother_set_target_index(param_smoother_t* sm,
                                           uint8_t index,
                                           int32_t new_value) {
    if (!smoother_index_valid(index)) return;
    smoother_set_target(sm,
                        smoother_page_from_index(index),
                        smoother_lane_from_index(index),
                        new_value);
}

/**
 * Immediately set a value without ramping.
 */
fast_inline void smoother_set_immediate(param_smoother_t* sm,
                                        uint8_t page,
                                        uint8_t param,
                                        int32_t new_value) {
    if (page >= PARAM_SMOOTHER_PAGES || param >= PARAM_SMOOTHER_LANES) return;

    const float value = smoother_norm_from_i32(new_value);

    float current[PARAM_SMOOTHER_LANES];
    float target[PARAM_SMOOTHER_LANES];
    float step[PARAM_SMOOTHER_LANES];
    uint32_t counter[PARAM_SMOOTHER_LANES];

    vst1q_f32(current, sm->current[page]);
    vst1q_f32(target, sm->target[page]);
    vst1q_f32(step, sm->step[page]);
    vst1q_u32(counter, sm->counter[page]);

    current[param] = value;
    target[param] = value;
    step[param] = 0.0f;
    counter[param] = 0;

    sm->current[page] = vld1q_f32(current);
    sm->target[page]  = vld1q_f32(target);
    sm->step[page]    = vld1q_f32(step);
    sm->counter[page] = vld1q_u32(counter);
}

/* -------------------------------------------------------------------------
 * Process
 * ------------------------------------------------------------------------- */

/**
 * Process one sample of smoothing for all parameters.
 *
 * Unlike the older version, this only applies step on lanes that are actively
 * ramping. This avoids relying on step being zero for inactive lanes.
 */
fast_inline void smoother_process(param_smoother_t* sm,
                                  float32x4_t smoothed[PARAM_SMOOTHER_PAGES]) {
    const uint32x4_t zero_u = vdupq_n_u32(0);
    const uint32x4_t one_u  = vdupq_n_u32(1);

    for (int page = 0; page < PARAM_SMOOTHER_PAGES; ++page) {
        uint32x4_t active = vcgtq_u32(sm->counter[page], zero_u);
        uint32x4_t finishing = vandq_u32(active, vcgeq_u32(one_u, sm->counter[page]));

        float32x4_t ramped = vaddq_f32(sm->current[page], sm->step[page]);
        sm->current[page] = vbslq_f32(active, ramped, sm->current[page]);

        sm->counter[page] = vbslq_u32(active,
                                       vsubq_u32(sm->counter[page], one_u),
                                       sm->counter[page]);

        // Snap on the final sample to remove accumulated step error.
        sm->current[page] = vbslq_f32(finishing, sm->target[page], sm->current[page]);
        sm->step[page] = vbslq_f32(finishing, vdupq_n_f32(0.0f), sm->step[page]);

        // Clamp for safety.
        sm->current[page] = vmaxq_f32(vdupq_n_f32(0.0f),
                                      vminq_f32(vdupq_n_f32(1.0f),
                                                sm->current[page]));

        smoothed[page] = sm->current[page];
    }
}

/**
 * Convenience process function when the caller does not need the whole output.
 */
fast_inline void smoother_process_inplace(param_smoother_t* sm) {
    float32x4_t dummy[PARAM_SMOOTHER_PAGES];
    smoother_process(sm, dummy);
}

/**
 * Check if any parameters are still smoothing.
 */
fast_inline uint32_t smoother_active(const param_smoother_t* sm) {
    uint32_t active = 0;

    for (int page = 0; page < PARAM_SMOOTHER_PAGES; ++page) {
        uint32x4_t counters = sm->counter[page];
        active |= vgetq_lane_u32(counters, 0);
        active |= vgetq_lane_u32(counters, 1);
        active |= vgetq_lane_u32(counters, 2);
        active |= vgetq_lane_u32(counters, 3);
    }

    return active;
}

/* -------------------------------------------------------------------------
 * Getters
 * ------------------------------------------------------------------------- */

fast_inline float smoother_get(const param_smoother_t* sm,
                               uint8_t page,
                               uint8_t param) {
    if (page >= PARAM_SMOOTHER_PAGES || param >= PARAM_SMOOTHER_LANES) return 0.0f;

    float vals[PARAM_SMOOTHER_LANES];
    vst1q_f32(vals, sm->current[page]);
    return vals[param];
}

fast_inline float smoother_get_index(const param_smoother_t* sm,
                                     uint8_t index) {
    if (!smoother_index_valid(index)) return 0.0f;
    return smoother_get(sm,
                        smoother_page_from_index(index),
                        smoother_lane_from_index(index));
}

fast_inline float smoother_get_scaled(const param_smoother_t* sm,
                                      uint8_t page,
                                      uint8_t param,
                                      float min_value,
                                      float max_value) {
    float norm = smoother_get(sm, page, param);
    return min_value + norm * (max_value - min_value);
}

fast_inline float smoother_get_bipolar(const param_smoother_t* sm,
                                       uint8_t page,
                                       uint8_t param) {
    float norm = smoother_get(sm, page, param);
    return norm * 2.0f - 1.0f;
}

// ========== UNIT TEST ==========
#ifdef TEST_SMOOTHING

#include <stdio.h>
#include <assert.h>
#include <math.h>

void test_smoothing_initialization() {
    param_smoother_t sm;
    int8_t params[PARAM_SMOOTHER_TOTAL];

    for (int i = 0; i < PARAM_SMOOTHER_TOTAL; ++i) {
        params[i] = (int8_t)(i * 4);
    }

    smoother_init(&sm, params);

    for (int page = 0; page < PARAM_SMOOTHER_PAGES; ++page) {
        float current[PARAM_SMOOTHER_LANES];
        float step[PARAM_SMOOTHER_LANES];
        vst1q_f32(current, sm.current[page]);
        vst1q_f32(step, sm.step[page]);

        for (int param = 0; param < PARAM_SMOOTHER_LANES; ++param) {
            float expected = smoother_norm_from_i32(params[page * PARAM_SMOOTHER_LANES + param]);
            assert(fabsf(current[param] - expected) < 0.001f);
            assert(step[param] == 0.0f);
        }
    }

    printf("Smoother initialization test PASSED\n");
}

void test_smoothing_ramp() {
    param_smoother_t sm;
    int8_t params[PARAM_SMOOTHER_TOTAL] = {0};

    smoother_init(&sm, params);
    smoother_set_target(&sm, 0, 0, 100);

    float last_value = 0.0f;

    for (int i = 0; i < SMOOTH_FRAMES + 10; ++i) {
        float32x4_t smoothed_vec[PARAM_SMOOTHER_PAGES];
        smoother_process(&sm, smoothed_vec);

        float vals[PARAM_SMOOTHER_LANES];
        vst1q_f32(vals, smoothed_vec[0]);
        float current = vals[0];

        if (i < SMOOTH_FRAMES - 1) {
            assert(current > last_value);
            assert(current < 1.0f);
        } else {
            assert(fabsf(current - 1.0f) < 0.001f);
        }

        last_value = current;
    }

    assert(smoother_active(&sm) == 0);
    printf("Smoothing ramp test PASSED\n");
}

void test_multiple_parameters() {
    param_smoother_t sm;
    int8_t params[PARAM_SMOOTHER_TOTAL] = {0};

    smoother_init(&sm, params);

    smoother_set_target(&sm, 0, 0, 100);
    smoother_set_target(&sm, 0, 1, 0);
    smoother_set_target(&sm, 1, 2, 50);

    for (int i = 0; i < SMOOTH_FRAMES; ++i) {
        float32x4_t smoothed_vec[PARAM_SMOOTHER_PAGES];
        smoother_process(&sm, smoothed_vec);

        float page0_vals[PARAM_SMOOTHER_LANES];
        vst1q_f32(page0_vals, smoothed_vec[0]);

        float page1_vals[PARAM_SMOOTHER_LANES];
        vst1q_f32(page1_vals, smoothed_vec[1]);

        assert(page0_vals[1] == 0.0f);
        assert(page1_vals[2] >= 0.0f);
    }

    printf("Multiple parameter smoothing test PASSED\n");
}

void test_bipolar_conversion() {
    param_smoother_t sm;
    int8_t params[PARAM_SMOOTHER_TOTAL] = {0};

    params[0] = 0;
    smoother_init(&sm, params);
    assert(fabsf(smoother_get_bipolar(&sm, 0, 0) + 1.0f) < 0.001f);

    params[0] = 50;
    smoother_init(&sm, params);
    assert(fabsf(smoother_get_bipolar(&sm, 0, 0)) < 0.001f);

    params[0] = 100;
    smoother_init(&sm, params);
    assert(fabsf(smoother_get_bipolar(&sm, 0, 0) - 1.0f) < 0.001f);

    printf("Bipolar conversion test PASSED\n");
}

#endif // TEST_SMOOTHING
