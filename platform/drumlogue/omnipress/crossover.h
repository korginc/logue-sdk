#pragma once
/*
 * File: crossover.h
 * 
 * Linkwitz-Riley 24dB/oct crossover filters
 * Perfect for multiband compression - zero phase shift at crossover
 * 
 * Based on SHARC Audio Elements biquad_filter.c architecture
 */

#include <arm_neon.h>
#include <math.h>

// Crossover filter state (4 parallel filters for stereo + future)
typedef struct {
    float32x4_t lpf_z1;      // Low-pass delay elements
    float32x4_t lpf_z2;
    float32x4_t hpf_z1;      // High-pass delay elements
    float32x4_t hpf_z2;
    float32x4_t lpf2_z1;     // Second stage for 24dB/oct
    float32x4_t lpf2_z2;
    float32x4_t hpf2_z1;
    float32x4_t hpf2_z2;
    
    float lpf_coeffs[5];      // Biquad coefficients (shared across channels)
    float hpf_coeffs[5];
} crossover_t;

// Initialize crossover at given frequency
fast_inline void crossover_init(crossover_t* xover, float freq_hz, float sample_rate) {
    float w0 = 2.0f * M_PI * freq_hz / sample_rate;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float Q = 0.707f;  // Butterworth Q for Linkwitz-Riley
    
    // Calculate alpha for biquad
    float alpha = sin_w0 / (2.0f * Q);
    
    // Low-pass coefficients (first stage)
    float b0 = (1.0f - cos_w0) / 2.0f;
    float b1 = 1.0f - cos_w0;
    float b2 = b0;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;
    
    // Normalize
    xover->lpf_coeffs[0] = b0 / a0;
    xover->lpf_coeffs[1] = b1 / a0;
    xover->lpf_coeffs[2] = b2 / a0;
    xover->lpf_coeffs[3] = a1 / a0;
    xover->lpf_coeffs[4] = a2 / a0;
    
    // High-pass coefficients (first stage)
    b0 = (1.0f + cos_w0) / 2.0f;
    b1 = -(1.0f + cos_w0);
    b2 = b0;
    
    xover->hpf_coeffs[0] = b0 / a0;
    xover->hpf_coeffs[1] = b1 / a0;
    xover->hpf_coeffs[2] = b2 / a0;
    xover->hpf_coeffs[3] = a1 / a0;
    xover->hpf_coeffs[4] = a2 / a0;
    
    // Reset states
    xover->lpf_z1 = vdupq_n_f32(0.0f);
    xover->lpf_z2 = vdupq_n_f32(0.0f);
    xover->hpf_z1 = vdupq_n_f32(0.0f);
    xover->hpf_z2 = vdupq_n_f32(0.0f);
    xover->lpf2_z1 = vdupq_n_f32(0.0f);
    xover->lpf2_z2 = vdupq_n_f32(0.0f);
    xover->hpf2_z1 = vdupq_n_f32(0.0f);
    xover->hpf2_z2 = vdupq_n_f32(0.0f);
}

// Process one sample through biquad (Direct Form I)
fast_inline float32x4_t biquad_process(float32x4_t in,
                                       float32x4_t* z1,
                                       float32x4_t* z2,
                                       const float* coeffs) {
    float32x4_t b0 = vdupq_n_f32(coeffs[0]);
    float32x4_t b1 = vdupq_n_f32(coeffs[1]);
    float32x4_t b2 = vdupq_n_f32(coeffs[2]);
    float32x4_t a1 = vdupq_n_f32(coeffs[3]);
    float32x4_t a2 = vdupq_n_f32(coeffs[4]);
    
    // Direct Form I
    float32x4_t out = vaddq_f32(vmulq_f32(in, b0), *z1);
    *z1 = vaddq_f32(vmlaq_f32(*z2, in, b1), vmulq_f32(out, a1));
    *z2 = vaddq_f32(vmulq_f32(in, b2), vmulq_f32(out, a2));
    
    return out;
}

// Process stereo through crossover (returns low, mid, high bands)
fast_inline void crossover_process(crossover_t* xover,
                                   float32x4_t in_l,
                                   float32x4_t in_r,
                                   float32x4_t* low_l,
                                   float32x4_t* low_r,
                                   float32x4_t* mid_l,
                                   float32x4_t* mid_r,
                                   float32x4_t* high_l,
                                   float32x4_t* high_r,
                                   float crossover_freq,
                                   float sample_rate) {
    
    // Update coefficients if frequency changed (can be optimized)
    static float last_freq = 0.0f;
    if (fabsf(crossover_freq - last_freq) > 1.0f) {
        crossover_init(xover, crossover_freq, sample_rate);
        last_freq = crossover_freq;
    }
    
    // Process left channel through low-pass (24dB/oct = two biquads in series)
    float32x4_t lpf1_l = biquad_process(in_l, &xover->lpf_z1, &xover->lpf_z2, xover->lpf_coeffs);
    float32x4_t lpf2_l = biquad_process(lpf1_l, &xover->lpf2_z1, &xover->lpf2_z2, xover->lpf_coeffs);
    
    // Process left channel through high-pass (24dB/oct)
    float32x4_t hpf1_l = biquad_process(in_l, &xover->hpf_z1, &xover->hpf_z2, xover->hpf_coeffs);
    float32x4_t hpf2_l = biquad_process(hpf1_l, &xover->hpf2_z1, &xover->hpf2_z2, xover->hpf_coeffs);
    
    // Right channel (using separate state but same coefficients)
    float32x4_t lpf1_r = biquad_process(in_r, &xover->lpf_z1, &xover->lpf_z2, xover->lpf_coeffs);
    float32x4_t lpf2_r = biquad_process(lpf1_r, &xover->lpf2_z1, &xover->lpf2_z2, xover->lpf_coeffs);
    
    float32x4_t hpf1_r = biquad_process(in_r, &xover->hpf_z1, &xover->hpf_z2, xover->hpf_coeffs);
    float32x4_t hpf2_r = biquad_process(hpf1_r, &xover->hpf2_z1, &xover->hpf2_z2, xover->hpf_coeffs);
    
    // Low band = low-pass filtered
    *low_l = lpf2_l;
    *low_r = lpf2_r;
    
    // High band = high-pass filtered
    *high_l = hpf2_l;
    *high_r = hpf2_r;
    
    // Mid band = all-pass minus low and high (complementary)
    *mid_l = vsubq_f32(vsubq_f32(in_l, lpf2_l), hpf2_l);
    *mid_r = vsubq_f32(vsubq_f32(in_r, lpf2_r), hpf2_r);
}