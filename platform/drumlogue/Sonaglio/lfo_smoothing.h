/**
 *  @file lfo_smoothing.h
 *  @brief Smooth LFO parameter changes to prevent zipper noise
 *
 *  Smooths rate and depth with short NEON ramps.
 *  Discrete target changes are delayed to the end of a short guard ramp.
 *
 *  Notes for current Sonaglio:
 *  - The active design is selector-based, but LFO smoothing is still useful.
 *  - Depth is bipolar.
 *  - Target is discrete and must not be interpolated.
 */

#ifndef C89DE9AE_A2F1_4CAE_B8F6_651DAE1130C7
#define C89DE9AE_A2F1_4CAE_B8F6_651DAE1130C7

#include <arm_neon.h>
#include <stdint.h>
#include <math.h>
#include "float_math.h"
#include "lfo_enhanced.h"

#define LFO_SMOOTH_FRAMES 48  // 1ms at 48kHz

typedef struct {
    // Current smoothed values
    float32x4_t current_rate1;
    float32x4_t current_rate2;
    float32x4_t current_depth1;
    float32x4_t current_depth2;
    uint32x4_t current_target1;
    uint32x4_t current_target2;

    // Target values
    float32x4_t target_rate1;
    float32x4_t target_rate2;
    float32x4_t target_depth1;
    float32x4_t target_depth2;
    uint32x4_t target_target1;
    uint32x4_t target_target2;

    // Ramp counters
    uint32x4_t rate_ramp1_counter;
    uint32x4_t rate_ramp2_counter;
    uint32x4_t depth_ramp1_counter;
    uint32x4_t depth_ramp2_counter;
    uint32x4_t target_ramp1_counter;
    uint32x4_t target_ramp2_counter;

    // Ramp steps
    float32x4_t rate_step1;
    float32x4_t rate_step2;
    float32x4_t depth_step1;
    float32x4_t depth_step2;
} lfo_smoother_t;

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

fast_inline uint32x4_t lfo_smoother_full_mask(uint32x4_t mask) {
    // Accept either 0/1 masks or full 0xFFFFFFFF masks.
    return vcgtq_u32(mask, vdupq_n_u32(0));
}

fast_inline float lfo_smoother_clamp_rate(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

fast_inline float lfo_smoother_clamp_depth(float x) {
    if (x < -1.0f) return -1.0f;
    if (x >  1.0f) return  1.0f;
    return x;
}

fast_inline uint8_t lfo_smoother_clamp_target(uint8_t t) {
    // LFO_TARGET_COUNT is provided by lfo_enhanced.h in the revised model.
#ifdef LFO_TARGET_COUNT
    if (t >= LFO_TARGET_COUNT) return LFO_TARGET_NONE;
#endif
    return t;
}

fast_inline float32x4_t lfo_smoother_ramp_step(float32x4_t current,
                                               float32x4_t target) {
    return vmulq_n_f32(vsubq_f32(target, current),
                       1.0f / (float)LFO_SMOOTH_FRAMES);
}

/**
 * Initialize LFO smoother
 */
fast_inline void lfo_smoother_init(lfo_smoother_t* sm) {
    sm->current_rate1 = vdupq_n_f32(0.01f);
    sm->current_rate2 = vdupq_n_f32(0.01f);
    sm->current_depth1 = vdupq_n_f32(0.0f);
    sm->current_depth2 = vdupq_n_f32(0.0f);
    sm->current_target1 = vdupq_n_u32(LFO_TARGET_NONE);
    sm->current_target2 = vdupq_n_u32(LFO_TARGET_NONE);

    sm->target_rate1 = sm->current_rate1;
    sm->target_rate2 = sm->current_rate2;
    sm->target_depth1 = sm->current_depth1;
    sm->target_depth2 = sm->current_depth2;
    sm->target_target1 = sm->current_target1;
    sm->target_target2 = sm->current_target2;

    sm->rate_ramp1_counter = vdupq_n_u32(0);
    sm->rate_ramp2_counter = vdupq_n_u32(0);
    sm->depth_ramp1_counter = vdupq_n_u32(0);
    sm->depth_ramp2_counter = vdupq_n_u32(0);
    sm->target_ramp1_counter = vdupq_n_u32(0);
    sm->target_ramp2_counter = vdupq_n_u32(0);

    sm->rate_step1 = vdupq_n_f32(0.0f);
    sm->rate_step2 = vdupq_n_f32(0.0f);
    sm->depth_step1 = vdupq_n_f32(0.0f);
    sm->depth_step2 = vdupq_n_f32(0.0f);
}

/**
 * Set new target rate with smoothing.
 *
 * new_rate is the normalized UI value, 0..1.
 */
fast_inline void lfo_smoother_set_rate(lfo_smoother_t* sm,
                                       uint8_t lfo_num,
                                       float new_rate,
                                       uint32x4_t voice_mask) {
    voice_mask = lfo_smoother_full_mask(voice_mask);

    float32x4_t new_rate_vec = vdupq_n_f32(lfo_smoother_clamp_rate(new_rate));

    if (lfo_num == 0) {
        sm->rate_step1 = vbslq_f32(voice_mask,
                                   lfo_smoother_ramp_step(sm->current_rate1,
                                                          new_rate_vec),
                                   sm->rate_step1);

        sm->target_rate1 = vbslq_f32(voice_mask, new_rate_vec, sm->target_rate1);
        sm->rate_ramp1_counter = vbslq_u32(voice_mask,
                                           vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                           sm->rate_ramp1_counter);
    } else {
        sm->rate_step2 = vbslq_f32(voice_mask,
                                   lfo_smoother_ramp_step(sm->current_rate2,
                                                          new_rate_vec),
                                   sm->rate_step2);

        sm->target_rate2 = vbslq_f32(voice_mask, new_rate_vec, sm->target_rate2);
        sm->rate_ramp2_counter = vbslq_u32(voice_mask,
                                           vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                           sm->rate_ramp2_counter);
    }
}

/**
 * Set new target depth with smoothing.
 *
 * new_depth is bipolar, -1..1.
 */
fast_inline void lfo_smoother_set_depth(lfo_smoother_t* sm,
                                        uint8_t lfo_num,
                                        float new_depth,
                                        uint32x4_t voice_mask) {
    voice_mask = lfo_smoother_full_mask(voice_mask);

    float32x4_t new_depth_vec = vdupq_n_f32(lfo_smoother_clamp_depth(new_depth));

    if (lfo_num == 0) {
        sm->depth_step1 = vbslq_f32(voice_mask,
                                    lfo_smoother_ramp_step(sm->current_depth1,
                                                           new_depth_vec),
                                    sm->depth_step1);

        sm->target_depth1 = vbslq_f32(voice_mask, new_depth_vec, sm->target_depth1);
        sm->depth_ramp1_counter = vbslq_u32(voice_mask,
                                            vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                            sm->depth_ramp1_counter);
    } else {
        sm->depth_step2 = vbslq_f32(voice_mask,
                                    lfo_smoother_ramp_step(sm->current_depth2,
                                                           new_depth_vec),
                                    sm->depth_step2);

        sm->target_depth2 = vbslq_f32(voice_mask, new_depth_vec, sm->target_depth2);
        sm->depth_ramp2_counter = vbslq_u32(voice_mask,
                                            vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                            sm->depth_ramp2_counter);
    }
}

/**
 * Set new target with a short guard ramp.
 *
 * Targets are discrete, so they are not interpolated. We delay the switch by
 * LFO_SMOOTH_FRAMES so modulation depth/rate changes can settle first and the
 * target change does not click as abruptly.
 */
fast_inline void lfo_smoother_set_target(lfo_smoother_t* sm,
                                         uint8_t lfo_num,
                                         uint8_t new_target,
                                         uint32x4_t voice_mask) {
    voice_mask = lfo_smoother_full_mask(voice_mask);

    uint32x4_t new_target_vec = vdupq_n_u32(lfo_smoother_clamp_target(new_target));

    if (lfo_num == 0) {
        sm->target_target1 = vbslq_u32(voice_mask, new_target_vec, sm->target_target1);
        sm->target_ramp1_counter = vbslq_u32(voice_mask,
                                             vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                             sm->target_ramp1_counter);
    } else {
        sm->target_target2 = vbslq_u32(voice_mask, new_target_vec, sm->target_target2);
        sm->target_ramp2_counter = vbslq_u32(voice_mask,
                                             vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                             sm->target_ramp2_counter);
    }
}

/* -------------------------------------------------------------------------
 * Process helpers
 * ------------------------------------------------------------------------- */

fast_inline void lfo_smoother_process_float_ramp(float32x4_t* current,
                                                 const float32x4_t* target,
                                                 const float32x4_t* step,
                                                 uint32x4_t* counter) {
    const uint32x4_t zero = vdupq_n_u32(0);
    const uint32x4_t one  = vdupq_n_u32(1);

    uint32x4_t active = vcgtq_u32(*counter, zero);
    uint32x4_t finishing = vandq_u32(active, vcgeq_u32(one, *counter));

    float32x4_t ramped = vaddq_f32(*current, *step);
    *current = vbslq_f32(active, ramped, *current);
    *counter = vbslq_u32(active, vsubq_u32(*counter, one), *counter);

    // Snap exactly on the final sample. This avoids accumulated step error.
    *current = vbslq_f32(finishing, *target, *current);
}

fast_inline void lfo_smoother_process_target_ramp(uint32x4_t* current,
                                                  const uint32x4_t* target,
                                                  uint32x4_t* counter) {
    const uint32x4_t zero = vdupq_n_u32(0);
    const uint32x4_t one  = vdupq_n_u32(1);

    uint32x4_t active = vcgtq_u32(*counter, zero);
    uint32x4_t finishing = vandq_u32(active, vcgeq_u32(one, *counter));

    *counter = vbslq_u32(active, vsubq_u32(*counter, one), *counter);
    *current = vbslq_u32(finishing, *target, *current);
}

/**
 * Process one sample of smoothing.
 */
fast_inline void lfo_smoother_process(lfo_smoother_t* sm) {
    lfo_smoother_process_float_ramp(&sm->current_rate1,
                                    &sm->target_rate1,
                                    &sm->rate_step1,
                                    &sm->rate_ramp1_counter);

    lfo_smoother_process_float_ramp(&sm->current_rate2,
                                    &sm->target_rate2,
                                    &sm->rate_step2,
                                    &sm->rate_ramp2_counter);

    lfo_smoother_process_float_ramp(&sm->current_depth1,
                                    &sm->target_depth1,
                                    &sm->depth_step1,
                                    &sm->depth_ramp1_counter);

    lfo_smoother_process_float_ramp(&sm->current_depth2,
                                    &sm->target_depth2,
                                    &sm->depth_step2,
                                    &sm->depth_ramp2_counter);

    lfo_smoother_process_target_ramp(&sm->current_target1,
                                     &sm->target_target1,
                                     &sm->target_ramp1_counter);

    lfo_smoother_process_target_ramp(&sm->current_target2,
                                     &sm->target_target2,
                                     &sm->target_ramp2_counter);
}

// ========== UNIT TEST ==========
#ifdef TEST_LFO_SMOOTHING

#include <stdio.h>
#include <assert.h>
#include <math.h>

void test_lfo_smoothing_ramp() {
    lfo_smoother_t sm;
    lfo_smoother_init(&sm);

    uint32x4_t all_voices = vdupq_n_u32(0xFFFFFFFF);

    lfo_smoother_set_rate(&sm, 0, 0.5f, all_voices);

    float last_rate = 0.01f;
    for (int i = 0; i < LFO_SMOOTH_FRAMES + 10; i++) {
        lfo_smoother_process(&sm);

        float current = vgetq_lane_f32(sm.current_rate1, 0);

        if (i < LFO_SMOOTH_FRAMES - 1) {
            assert(current > last_rate);
            assert(current < 0.5f);
        } else {
            assert(fabsf(current - 0.5f) < 0.001f);
        }

        last_rate = current;
    }

    printf("LFO smoothing ramp test PASSED\n");
}

void test_bipolar_smoothing() {
    lfo_smoother_t sm;
    lfo_smoother_init(&sm);

    uint32x4_t all_voices = vdupq_n_u32(0xFFFFFFFF);

    lfo_smoother_set_depth(&sm, 0, -0.8f, all_voices);

    float last_depth = 0.0f;
    for (int i = 0; i < LFO_SMOOTH_FRAMES + 10; i++) {
        lfo_smoother_process(&sm);

        float current = vgetq_lane_f32(sm.current_depth1, 0);

        if (i < LFO_SMOOTH_FRAMES - 1) {
            assert(current < last_depth);
            assert(current > -0.8f);
        } else {
            assert(fabsf(current - (-0.8f)) < 0.001f);
        }

        last_depth = current;
    }

    printf("Bipolar smoothing test PASSED\n");
}

void test_target_switch_delay() {
    lfo_smoother_t sm;
    lfo_smoother_init(&sm);

    uint32x4_t all_voices = vdupq_n_u32(0xFFFFFFFF);

    lfo_smoother_set_target(&sm, 0, LFO_TARGET_PITCH, all_voices);

    for (int i = 0; i < LFO_SMOOTH_FRAMES - 1; ++i) {
        lfo_smoother_process(&sm);
        assert(vgetq_lane_u32(sm.current_target1, 0) == LFO_TARGET_NONE);
    }

    lfo_smoother_process(&sm);
    assert(vgetq_lane_u32(sm.current_target1, 0) == LFO_TARGET_PITCH);

    printf("Target switch delay test PASSED\n");
}

#endif // TEST_LFO_SMOOTHING

#endif // C89DE9AE_A2F1_4CAE_B8F6_651DAE1130C7
