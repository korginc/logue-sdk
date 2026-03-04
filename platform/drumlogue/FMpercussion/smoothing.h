/**
 *  @file smoothing.h
 *  @brief NEON-optimized parameter smoothing to prevent zipper noise
 *
 *  Linear interpolation over 1ms ramp time (48 samples at 48kHz)
 */

#pragma once

#include <arm_neon.h>
#include <stdint.h>

#define SMOOTH_FRAMES 48  // 1ms at 48kHz

typedef struct {
    float32x4_t current[6];    // Current smoothed values (6 pages × 4 params)
    float32x4_t target[6];      // Target values
    uint32x4_t counter[6];      // Remaining frames in ramp
    float32x4_t step[6];        // Per-frame increment
} param_smoother_t;

/**
 * Initialize smoother with current values
 */
fast_inline void smoother_init(param_smoother_t* sm, const uint8_t* params) {
    for (int page = 0; page < 6; page++) {
        float values[4];
        for (int i = 0; i < 4; i++) {
            values[i] = params[page * 4 + i] / 100.0f;  // Normalize to 0-1
        }
        sm->current[page] = vld1q_f32(values);
        sm->target[page] = sm->current[page];
        sm->counter[page] = vdupq_n_u32(0);
    }
}

/**
 * Set new target value with smoothing
 */
fast_inline void smoother_set_target(param_smoother_t* sm, 
                                     uint8_t page, uint8_t param, 
                                     uint8_t new_value) {
    // Extract current target vector
    float targets[4];
    vst1q_f32(targets, sm->target[page]);
    
    // Update specific parameter
    targets[param] = new_value / 100.0f;
    
    // Store back
    sm->target[page] = vld1q_f32(targets);
    
    // Start ramp if not already ramping
    uint32_t counters[4];
    vst1q_u32(counters, sm->counter[page]);
    
    if (counters[param] == 0) {
        // Calculate step for this parameter
        float current[4];
        vst1q_f32(current, sm->current[page]);
        
        float diff = targets[param] - current[param];
        sm->step[page] = vsetq_lane_f32(diff / SMOOTH_FRAMES, 
                                        sm->step[page], param);
        
        counters[param] = SMOOTH_FRAMES;
        sm->counter[page] = vld1q_u32(counters);
    }
}

/**
 * Process one sample of smoothing for all parameters
 * Returns smoothed parameter vectors for all 6 pages
 */
fast_inline void smoother_process(param_smoother_t* sm, 
                                  float32x4_t smoothed[6]) {
    for (int page = 0; page < 6; page++) {
        // Check if any parameters still ramping
        uint32x4_t active = vcgtq_u32(sm->counter[page], vdupq_n_u32(0));
        
        // If any active, step toward target
        if (vget_lane_u32(vreinterpret_u32_u8(vmovn_u32(active)), 0)) {
            sm->current[page] = vaddq_f32(sm->current[page], sm->step[page]);
            
            // Decrement counters
            sm->counter[page] = vsubq_u32(sm->counter[page], vdupq_n_u32(1));
            
            // Clamp to target when done
            uint32x4_t done = vceqq_u32(sm->counter[page], vdupq_n_u32(0));
            sm->current[page] = vbslq_f32(done, 
                                          sm->target[page], 
                                          sm->current[page]);
        }
        
        smoothed[page] = sm->current[page];
    }
}

// ========== UNIT TEST ==========
#ifdef TEST_SMOOTHING

void test_smoothing_ramp() {
    param_smoother_t sm;
    uint8_t params[24] = {0};
    
    // Initialize all to 0
    smoother_init(&sm, params);
    
    // Set target to 100% on page 0, param 0
    smoother_set_target(&sm, 0, 0, 100);
    
    float last_value = 0;
    for (int i = 0; i < SMOOTH_FRAMES + 10; i++) {
        float32x4_t smoothed[6];
        smoother_process(&sm, smoothed);
        
        float current = vgetq_lane_f32(smoothed[0], 0);
        
        if (i < SMOOTH_FRAMES) {
            // Should be ramping
            assert(current > last_value);
            assert(current <= 1.0f);
        } else {
            // Should be at target
            assert(fabsf(current - 1.0f) < 0.001f);
        }
        
        last_value = current;
    }
    
    printf("Parameter smoothing test PASSED\n");
}

#endif