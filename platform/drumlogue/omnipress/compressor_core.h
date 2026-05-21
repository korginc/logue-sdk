#pragma once
/*
 * File: compressor_core.h
 *
 * Core compressor algorithms with negative ratio support
 * Implements Omnipressor-style reverse compression
 */

#include <arm_neon.h>
#include <math.h>
#include "float_math.h"

// Compressor state structure
typedef struct {
    // Envelope follower state
    float32x4_t env_state;      // Current envelope value (linear)
    float32x4_t rms_state;      // RMS squared sum
    float32x4_t gain_state;     // Current gain reduction (dB)
    float32x4_t gain_target;    // Target gain reduction (dB)

    // Detection mode
    uint32_t mode;               // 0=peak, 1=RMS, 2=blend
} compressor_t;

// Initialize compressor
fast_inline void compressor_init(compressor_t* comp) {
    comp->env_state = vdupq_n_f32(0.0f);
    comp->rms_state = vdupq_n_f32(0.0f);
    comp->gain_state = vdupq_n_f32(0.0f);
    comp->gain_target = vdupq_n_f32(0.0f);
    comp->mode = 0;
}

fast_inline void compressor_reset(compressor_t* comp) {
    compressor_init(comp);
}

// RMS processing (simplified - uses single pole on squared input - Fixed for sequential IIR correctness)
fast_inline float32x4_t compressor_rms_process(compressor_t* comp,
                                               float32x4_t squared) {
    float squares[NEON_LANES];
    float out[NEON_LANES];
    vst1q_f32(squares, squared);

    // Carry over the scalar history from the last lane of the previous block
    float state = vgetq_lane_f32(comp->rms_state, 3);

    // One-pole lowpass with 10ms time constant (simplified)
    for (int i = 0; i < NEON_LANES; ++i) {
        state = squares[i] * 0.01f + state * 0.99f;
        out[i] = fasterSqrt(state);
    }
    return vld1q_f32(out);
}

// Gain computer with negative ratio support
fast_inline float32x4_t compressor_calc_gain(compressor_t* comp,
                                             float32x4_t envelope,
                                             float thresh_db,
                                             float ratio) {
    (void)comp;  // State not needed for static gain curve; kept for API consistency
    // Convert envelope to dB (20 * log10(envelope))
    // Using approximation: log10(x) ≈ log2(x) * 0.30103
    uint32x4_t exp_mask = vdupq_n_u32(0x7F800000);
    uint32x4_t mant_mask = vdupq_n_u32(0x007FFFFF);

    // Extract exponent and mantissa for log2 approximation
    uint32x4_t u_envelope = vreinterpretq_u32_f32(envelope);
    uint32x4_t exp = vandq_u32(u_envelope, exp_mask);
    uint32x4_t mant = vandq_u32(u_envelope, mant_mask);

    // log2(envelope) ≈ exponent - 127 + log2(1 + mant/2^23)
    float32x4_t exp_f = vcvtq_f32_u32(vshrq_n_u32(exp, 23));
    float32x4_t mant_f = vcvtq_f32_u32(mant);
    float32x4_t log2_mant = vmulq_f32(mant_f, vdupq_n_f32(1.0f / (1 << 23)));

    float32x4_t log2_env = vaddq_f32(vsubq_f32(exp_f, vdupq_n_f32(127.0f)), log2_mant);

    // Convert to dB: 20 * log10(envelope) = 20 * log2(envelope) * log10(2)
    float32x4_t db_env = vmulq_f32(log2_env, vdupq_n_f32(6.0206f));  // 20 * log10(2)

    // Compute gain reduction (standard compressor formula)
    // GR_dB = -(excess * (ratio - 1) / ratio), where excess = env_dB - threshold_dB
    // For negative ratios, this creates upward expansion (Omnipressor-style)

    float32x4_t thresh_vec = vdupq_n_f32(thresh_db);
    // Calculate excess above threshold (positive when signal exceeds threshold)
    float32x4_t excess = vsubq_f32(db_env, thresh_vec);
    excess = vmaxq_f32(excess, vdupq_n_f32(0.0f)); // Clamp: only act above threshold

    float32x4_t ratio_vec = vdupq_n_f32(ratio);
    float32x4_t one_vec = vdupq_n_f32(1.0f);

    // FIX 2: Snap near-zero ratios to exactly 0.0f to trigger the hard-limit mask
    // Prevents massive upward expansion explosions (e.g. +1000dB)
    uint32x4_t near_zero = vcltq_f32(vabsq_f32(ratio_vec), vdupq_n_f32(0.01f));
    ratio_vec = vbslq_f32(near_zero, vdupq_n_f32(0.0f), ratio_vec);

    // Calculate ratio minus one for gain reduction
    float32x4_t ratio_minus_one = vsubq_f32(ratio_vec, one_vec);
    float32x4_t gain_red_num = vmulq_f32(excess, ratio_minus_one);
    float32x4_t gain_red = fast_div_neon(gain_red_num, ratio_vec);

    // For ratio=0, hard limit (infinite gain reduction above threshold)
    // gain_red is negated on return, so +100 here → -100 dB (silence above threshold)
    uint32x4_t ratio_zero = vceqq_f32(ratio_vec, vdupq_n_f32(0.0f));
    gain_red = vbslq_f32(ratio_zero, vdupq_n_f32(100.0f), gain_red);

    // For negative ratios, we get expansion (gain reduction becomes negative)
    // which actually increases gain - this is the Omnipressor magic

    return vnegq_f32(gain_red);  // Negative because we're reducing gain
}

// Attack/release smoothing (Fixed for sequential IIR correctness)
fast_inline float32x4_t compressor_smooth(compressor_t* comp,
                                          float32x4_t target_db,
                                          float attack_coeff,
                                          float release_coeff) {
    float targets[NEON_LANES];
    float out[NEON_LANES];
    vst1q_f32(targets, target_db);

    // Carry over the scalar history from the last lane of the previous block
    float state = vgetq_lane_f32(comp->gain_state, 3);

    // originally there was (targets[i] < state) to get either attack or release coeff,
    // but this causes branching which is bad for SIMD performance.
    // 1. Pre-calculate these constants OUTSIDE the loop
    const float coeff_avg  = 0.5f * (attack_coeff + release_coeff);
    const float coeff_diff = 0.5f * (attack_coeff - release_coeff);

    // 2. Run the branchless loop
    for (int i = 0; i < NEON_LANES; ++i) {
        float diff = state - targets[i];

        // std::fabs is branchless on modern hardware (just clears the sign bit).
        float product = (coeff_avg * diff) + (coeff_diff * fabsf(diff));

        state = targets[i] + product;
        out[i] = state;
    }

    comp->gain_state = vld1q_f32(out);
    return comp->gain_state;
}