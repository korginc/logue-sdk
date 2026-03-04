#pragma once
/*
 * File: filters.h
 * 
 * NEON-optimized filters for sidechain processing
 * Includes one-pole HPF and smoothing filters
 */

#include <arm_neon.h>

// One-pole HPF structure
typedef struct {
    float32x4_t z1;  // Delay element
} hpf_t;

// Initialize HPF
fast_inline void hpf_init(hpf_t* hpf) {
    hpf->z1 = vdupq_n_f32(0.0f);
}

fast_inline void hpf_reset(hpf_t* hpf) {
    hpf_init(hpf);
}

// Process 4 samples through HPF
// y[n] = a * (x[n] - x[n-1] + y[n-1])
// where a = 1 / (1 + 1/(2πfc/fs))
fast_inline float32x4_t hpf_process_4(hpf_t* hpf,
                                       float32x4_t x,
                                       float fc_norm) {
    // Calculate coefficient for cutoff frequency
    float w = 2.0f * M_PI * fc_norm;
    float a = w / (1.0f + w);  // Simple approximation
    
    float32x4_t a_vec = vdupq_n_f32(a);
    float32x4_t one_vec = vdupq_n_f32(1.0f);
    
    // y[n] = a * (x[n] - x[n-1]) + (1-a) * y[n-1]
    // But we'll use the standard form for stability
    
    // Difference
    float32x4_t diff = vsubq_f32(x, hpf->z1);
    
    // Output
    float32x4_t y = vaddq_f32(vmulq_f32(diff, a_vec),
                               vmulq_f32(hpf->z1, vsubq_f32(one_vec, a_vec)));
    
    // Update state
    hpf->z1 = x;
    
    return y;
}

// Simple one-pole lowpass for smoothing
typedef struct {
    float32x4_t z1;
} lpf_t;

fast_inline float32x4_t lpf_process(lpf_t* lpf,
                                     float32x4_t x,
                                     float coeff) {
    float32x4_t a = vdupq_n_f32(coeff);
    float32x4_t b = vdupq_n_f32(1.0f - coeff);
    
    float32x4_t y = vaddq_f32(vmulq_f32(x, a), vmulq_f32(lpf->z1, b));
    lpf->z1 = y;
    
    return y;
}