#pragma once
/*
 * File: compressor_core.h
 * 
 * Core compressor algorithms with negative ratio support
 * Implements Omnipressor-style reverse compression
 */

#include <arm_neon.h>
#include <math.h>

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

// RMS processing (simplified - uses single pole on squared input)
fast_inline float32x4_t compressor_rms_process(compressor_t* comp, 
                                                float32x4_t squared) {
    // One-pole lowpass with 10ms time constant (simplified)
    float32x4_t alpha = vdupq_n_f32(0.01f);
    float32x4_t one_minus_alpha = vdupq_n_f32(0.99f);
    
    comp->rms_state = vaddq_f32(vmulq_f32(squared, alpha),
                                 vmulq_f32(comp->rms_state, one_minus_alpha));
    
    // Return square root (approximate)
    return vsqrtq_f32(comp->rms_state);
}

// Gain computer with negative ratio support
fast_inline float32x4_t compressor_calc_gain(compressor_t* comp,
                                              float32x4_t envelope,
                                              float thresh_db,
                                              float ratio) {
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
    
    // Compute gain reduction
    // GR = (threshold - envelope) * (ratio - 1) / ratio
    // For negative ratios, this creates expansion or reverse compression
    
    float32x4_t thresh_vec = vdupq_n_f32(thresh_db);
    float32x4_t ratio_vec = vdupq_n_f32(ratio);
    float32x4_t one_vec = vdupq_n_f32(1.0f);
    
    // Calculate excess above threshold
    float32x4_t excess = vsubq_f32(thresh_vec, db_env);
    excess = vmaxq_f32(excess, vdupq_n_f32(0.0f));  // Only above threshold
    
    // Special case: ratio = 0 acts as hard limiter
    uint32x4_t ratio_zero = vceqq_f32(ratio_vec, vdupq_n_f32(0.0f));
    
    // gain_reduction = excess * (ratio - 1) / ratio
    float32x4_t ratio_minus_one = vsubq_f32(ratio_vec, one_vec);
    float32x4_t gain_red_num = vmulq_f32(excess, ratio_minus_one);
    float32x4_t gain_red = vdivq_f32(gain_red_num, ratio_vec);
    
    // For ratio=0, hard limit (infinite gain reduction above threshold)
    gain_red = vbslq_f32(ratio_zero, vdupq_n_f32(-100.0f), gain_red);
    
    // For negative ratios, we get expansion (gain reduction becomes negative)
    // which actually increases gain - this is the Omnipressor magic
    
    return vnegq_f32(gain_red);  // Negative because we're reducing gain
}

// Attack/release smoothing
fast_inline float32x4_t compressor_smooth(compressor_t* comp,
                                           float32x4_t target_db,
                                           float attack_coeff,
                                           float release_coeff) {
    // Determine if we're attacking or releasing
    uint32x4_t attacking = vcltq_f32(target_db, comp->gain_state);
    
    // Select appropriate coefficient
    float32x4_t coeff = vbslq_f32(attacking,
                                   vdupq_n_f32(attack_coeff),
                                   vdupq_n_f32(release_coeff));
    
    // One-pole smoothing
    float32x4_t one_minus_coeff = vsubq_f32(vdupq_n_f32(1.0f), coeff);
    comp->gain_state = vaddq_f32(vmulq_f32(target_db, one_minus_coeff),
                                  vmulq_f32(comp->gain_state, coeff));
    
    return comp->gain_state;
}