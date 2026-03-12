/**
 *  @file lfo_smoothing.h
 *  @brief Smooth LFO parameter changes to prevent zipper noise
 *
 *  Implements smooth transitions for rate, depth, and target changes
 *  Uses linear interpolation over configurable ramp time
 */

#pragma once

#include <arm_neon.h>
#include <stdint.h>

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

    // Zero all counters
    sm->rate_ramp1_counter = vdupq_n_u32(0);
    sm->rate_ramp2_counter = vdupq_n_u32(0);
    sm->depth_ramp1_counter = vdupq_n_u32(0);
    sm->depth_ramp2_counter = vdupq_n_u32(0);
    sm->target_ramp1_counter = vdupq_n_u32(0);
    sm->target_ramp2_counter = vdupq_n_u32(0);
}

/**
 * Set new target rate with smoothing
 */
fast_inline void lfo_smoother_set_rate(lfo_smoother_t* sm,
                                       uint8_t lfo_num,
                                       float new_rate,
                                       uint32x4_t voice_mask) {
    float32x4_t new_rate_vec = vdupq_n_f32(new_rate);

    if (lfo_num == 0) {
        // Calculate step for each voice
        float32x4_t diff = vsubq_f32(new_rate_vec, sm->current_rate1);
        sm->rate_step1 = vbslq_f32($1,
                                   vdivq_f32(diff, vdupq_n_f32(LFO_SMOOTH_FRAMES)),
                                   sm->rate_step1);

        // Set target and start ramp
        sm->target_rate1 = vbslq_f32(voice_mask,
                                     new_rate_vec,
                                     sm->target_rate1);
        sm->rate_ramp1_counter = vbslq_u32(voice_mask,
                                           vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                           sm->rate_ramp1_counter);
    } else {
        float32x4_t diff = vsubq_f32(new_rate_vec, sm->current_rate2);
        sm->rate_step2 = vbslq_f32(voice_mask,
                                   vdivq_f32(diff, vdupq_n_f32(LFO_SMOOTH_FRAMES)),
                                   sm->rate_step2);

        sm->target_rate2 = vbslq_f32(voice_mask,
                                     new_rate_vec,
                                     sm->target_rate2);
        sm->rate_ramp2_counter = vbslq_u32(voice_mask,
                                           vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                           sm->rate_ramp2_counter);
    }
}

/**
 * Set new target depth with smoothing (bipolar)
 */
fast_inline void lfo_smoother_set_depth(lfo_smoother_t* sm,
                                        uint8_t lfo_num,
                                        float new_depth,
                                        uint32x4_t voice_mask) {
    float32x4_t new_depth_vec = vdupq_n_f32(new_depth);

    if (lfo_num == 0) {
        float32x4_t diff = vsubq_f32(new_depth_vec, sm->current_depth1);
        sm->depth_step1 = vbslq_f32(voice_mask,
                                    vdivq_f32(diff, vdupq_n_f32(LFO_SMOOTH_FRAMES)),
                                    sm->depth_step1);

        sm->target_depth1 = vbslq_f32(voice_mask,
                                      new_depth_vec,
                                      sm->target_depth1);
        sm->depth_ramp1_counter = vbslq_u32(voice_mask,
                                            vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                            sm->depth_ramp1_counter);
    } else {
        float32x4_t diff = vsubq_f32(new_depth_vec, sm->current_depth2);
        sm->depth_step2 = vbslq_f32(voice_mask,
                                    vdivq_f32(diff, vdupq_n_f32(LFO_SMOOTH_FRAMES)),
                                    sm->depth_step2);

        sm->target_depth2 = vbslq_f32(voice_mask,
                                      new_depth_vec,
                                      sm->target_depth2);
        sm->depth_ramp2_counter = vbslq_u32(voice_mask,
                                            vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                            sm->depth_ramp2_counter);
    }
}

/**
 * Set new target with smoothing (for discrete targets)
 * Uses exponential moving average for smooth transitions
 */
fast_inline void lfo_smoother_set_target(lfo_smoother_t* sm,
                                         uint8_t lfo_num,
                                         uint8_t new_target,
                                         uint32x4_t voice_mask) {
    uint32x4_t new_target_vec = vdupq_n_u32(new_target);

    if (lfo_num == 0) {
        sm->target_target1 = vbslq_u32(voice_mask,
                                       new_target_vec,
                                       sm->target_target1);
        sm->target_ramp1_counter = vbslq_u32(voice_mask,
                                             vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                             sm->target_ramp1_counter);
    } else {
        sm->target_target2 = vbslq_u32(voice_mask,
                                       new_target_vec,
                                       sm->target_target2);
        sm->target_ramp2_counter = vbslq_u32(voice_mask,
                                             vdupq_n_u32(LFO_SMOOTH_FRAMES),
                                             sm->target_ramp2_counter);
    }
}

/**
 * Process one sample of smoothing
 */
fast_inline void lfo_smoother_process(lfo_smoother_t* sm) {
    // Smooth rate1
    uint32x4_t rate1_active = vcgtq_u32(sm->rate_ramp1_counter, vdupq_n_u32(0));
    sm->current_rate1 = vbslq_f32(rate1_active,
                                  vaddq_f32(sm->current_rate1, sm->rate_step1),
                                  sm->current_rate1);
    sm->rate_ramp1_counter = vbslq_u32(rate1_active,
                                       vsubq_u32(sm->rate_ramp1_counter, vdupq_n_u32(1)),
                                       sm->rate_ramp1_counter);

    // When done, snap to target
    uint32x4_t rate1_done = vceqq_u32(sm->rate_ramp1_counter, vdupq_n_u32(0));
    sm->current_rate1 = vbslq_f32(rate1_done,
                                  sm->target_rate1,
                                  sm->current_rate1);

    // Smooth rate2 (similar)
    uint32x4_t rate2_active = vcgtq_u32(sm->rate_ramp2_counter, vdupq_n_u32(0));
    sm->current_rate2 = vbslq_f32(rate2_active,
                                  vaddq_f32(sm->current_rate2, sm->rate_step2),
                                  sm->current_rate2);
    sm->rate_ramp2_counter = vbslq_u32(rate2_active,
                                       vsubq_u32(sm->rate_ramp2_counter, vdupq_n_u32(1)),
                                       sm->rate_ramp2_counter);

    uint32x4_t rate2_done = vceqq_u32(sm->rate_ramp2_counter, vdupq_n_u32(0));
    sm->current_rate2 = vbslq_f32(rate2_done,
                                  sm->target_rate2,
                                  sm->current_rate2);

    // Smooth depth1 (bipolar)
    uint32x4_t depth1_active = vcgtq_u32(sm->depth_ramp1_counter, vdupq_n_u32(0));
    sm->current_depth1 = vbslq_f32(depth1_active,
                                   vaddq_f32(sm->current_depth1, sm->depth_step1),
                                   sm->current_depth1);
    sm->depth_ramp1_counter = vbslq_u32(depth1_active,
                                        vsubq_u32(sm->depth_ramp1_counter, vdupq_n_u32(1)),
                                        sm->depth_ramp1_counter);

    uint32x4_t depth1_done = vceqq_u32(sm->depth_ramp1_counter, vdupq_n_u32(0));
    sm->current_depth1 = vbslq_f32(depth1_done,
                                   sm->target_depth1,
                                   sm->current_depth1);

    // Smooth depth2
    uint32x4_t depth2_active = vcgtq_u32(sm->depth_ramp2_counter, vdupq_n_u32(0));
    sm->current_depth2 = vbslq_f32(depth2_active,
                                   vaddq_f32(sm->current_depth2, sm->depth_step2),
                                   sm->current_depth2);
    sm->depth_ramp2_counter = vbslq_u32(depth2_active,
                                        vsubq_u32(sm->depth_ramp2_counter, vdupq_n_u32(1)),
                                        sm->depth_ramp2_counter);

    uint32x4_t depth2_done = vceqq_u32(sm->depth_ramp2_counter, vdupq_n_u32(0));
    sm->current_depth2 = vbslq_f32(depth2_done,
                                   sm->target_depth2,
                                   sm->current_depth2);

    // For targets, we can't interpolate - just switch when ramp completes
    uint32x4_t target1_done = vceqq_u32(sm->target_ramp1_counter, vdupq_n_u32(0));
    sm->current_target1 = vbslq_u32(target1_done,
                                    sm->target_target1,
                                    sm->current_target1);
    sm->target_ramp1_counter = vbslq_u32(vmvnq_u32(target1_done),
                                         vsubq_u32(sm->target_ramp1_counter, vdupq_n_u32(1)),
                                         sm->target_ramp1_counter);

    uint32x4_t target2_done = vceqq_u32(sm->target_ramp2_counter, vdupq_n_u32(0));
    sm->current_target2 = vbslq_u32(target2_done,
                                    sm->target_target2,
                                    sm->current_target2);
    sm->target_ramp2_counter = vbslq_u32(vmvnq_u32(target2_done),
                                         vsubq_u32(sm->target_ramp2_counter, vdupq_n_u32(1)),
                                         sm->target_ramp2_counter);
}

// ========== UNIT TEST ==========
#ifdef TEST_LFO_SMOOTHING

void test_lfo_smoothing_ramp() {
    lfo_smoother_t sm;
    lfo_smoother_init(&sm);

    uint32x4_t all_voices = vdupq_n_u32(0xFFFFFFFF);

    // Set new rate from 0.01 to 0.5
    lfo_smoother_set_rate(&sm, 0, 0.5f, all_voices);

    float last_rate = 0.01f;
    for (int i = 0; i < LFO_SMOOTH_FRAMES + 10; i++) {
        lfo_smoother_process(&sm);

        float current = vgetq_lane_f32(sm.current_rate1, 0);

        if (i < LFO_SMOOTH_FRAMES) {
            // Should be increasing
            assert(current > last_rate);
            assert(current < 0.5f);
        } else {
            // Should be at target
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

    // Set depth from 0.0 to -0.8 (negative)
    lfo_smoother_set_depth(&sm, 0, -0.8f, all_voices);

    float last_depth = 0.0f;
    for (int i = 0; i < LFO_SMOOTH_FRAMES + 10; i++) {
        lfo_smoother_process(&sm);

        float current = vgetq_lane_f32(sm.current_depth1, 0);

        if (i < LFO_SMOOTH_FRAMES) {
            // Should be decreasing
            assert(current < last_depth);
            assert(current > -0.8f);
        } else {
            // Should be at target
            assert(fabsf(current + 0.8f) < 0.001f);
        }

        last_depth = current;
    }

    printf("Bipolar smoothing test PASSED\n");
}

#endif