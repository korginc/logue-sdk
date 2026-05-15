#pragma once
/*
 * File: crossover.h
 *
 * Linkwitz-Riley 24dB/oct crossover filters
 * Perfect for multiband compression - zero phase shift at crossover
 *
 * FIXED (IIR correctness):
 * - biquad states changed from float32x4_t (4 independent histories) to float
 *   (single scalar state).  The old NEON implementation processed 4 consecutive
 *   time samples simultaneously through the IIR, breaking the feedback chain and
 *   creating 4 diverging filter histories that contaminated the band outputs with
 *   cross-band energy → audible harmonic distortion.
 * - biquad_process4() now processes 4 samples sequentially (correct IIR feedback).
 * - crossover_process() guard changed from crossover_init (zeros states → click)
 *   to crossover_update_coeffs (coefficients only, no state reset).
 */

#include "constants.h"
#include <arm_neon.h>
#include <math.h>

// Crossover filter state — scalar biquad histories for correct IIR feedback chain
typedef struct {
    float l_lpf_z1,  l_lpf_z2;
    float l_lpf2_z1, l_lpf2_z2;
    float l_hpf_z1,  l_hpf_z2;
    float l_hpf2_z1, l_hpf2_z2;

    float r_lpf_z1,  r_lpf_z2;
    float r_lpf2_z1, r_lpf2_z2;
    float r_hpf_z1,  r_hpf_z2;
    float r_hpf2_z1, r_hpf2_z2;

    float lpf_coeffs[5];   // b0 b1 b2 a1 a2 (normalized)
    float hpf_coeffs[5];
    float last_freq;
} crossover_t;

// Compute and store biquad coefficients — does NOT reset states.
fast_inline void crossover_update_coeffs(crossover_t* xover, float freq_hz, float sample_rate) {
    float w0 = 2.0f * M_PI * freq_hz / sample_rate;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float Q = 0.707f;
    float alpha = sin_w0 / (2.0f * Q);

    float b0 = (1.0f - cos_w0) / 2.0f;
    float b1 = 1.0f - cos_w0;
    float b2 = b0;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    xover->lpf_coeffs[0] = b0 / a0;
    xover->lpf_coeffs[1] = b1 / a0;
    xover->lpf_coeffs[2] = b2 / a0;
    xover->lpf_coeffs[3] = a1 / a0;
    xover->lpf_coeffs[4] = a2 / a0;

    b0 = (1.0f + cos_w0) / 2.0f;
    b1 = -(1.0f + cos_w0);
    b2 = b0;

    xover->hpf_coeffs[0] = b0 / a0;
    xover->hpf_coeffs[1] = b1 / a0;
    xover->hpf_coeffs[2] = b2 / a0;
    xover->hpf_coeffs[3] = a1 / a0;
    xover->hpf_coeffs[4] = a2 / a0;

    xover->last_freq = freq_hz;
}

// Initialize crossover — also resets all filter states (startup only).
fast_inline void crossover_init(crossover_t* xover, float freq_hz, float sample_rate) {
    crossover_update_coeffs(xover, freq_hz, sample_rate);

    xover->l_lpf_z1  = xover->l_lpf_z2  = 0.0f;
    xover->l_lpf2_z1 = xover->l_lpf2_z2 = 0.0f;
    xover->l_hpf_z1  = xover->l_hpf_z2  = 0.0f;
    xover->l_hpf2_z1 = xover->l_hpf2_z2 = 0.0f;
    xover->r_lpf_z1  = xover->r_lpf_z2  = 0.0f;
    xover->r_lpf2_z1 = xover->r_lpf2_z2 = 0.0f;
    xover->r_hpf_z1  = xover->r_hpf_z2  = 0.0f;
    xover->r_hpf2_z1 = xover->r_hpf2_z2 = 0.0f;
}

// Process 4 consecutive time samples through a biquad (Transposed Direct Form II).
// Scalar state ensures correct IIR feedback: y[n] feeds back into z1 before y[n+1].
fast_inline void biquad_process4(float* z1, float* z2,
                                  const float* c,
                                  const float* in, float* out) {
    const float b0 = c[0], b1 = c[1], b2 = c[2], a1 = c[3], a2 = c[4];
    float lz1 = *z1, lz2 = *z2;
    for (int i = 0; i < 4; ++i) {
        const float x = in[i];
        const float y = b0 * x + lz1;
        lz1 = b1 * x - a1 * y + lz2;
        lz2 = b2 * x - a2 * y;
        out[i] = y;
    }
    *z1 = lz1; *z2 = lz2;
}

// Process stereo through crossover — L and R use fully independent filter states.
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
    // Update coefficients only (no state reset) when frequency changes.
    if (fabsf(crossover_freq - xover->last_freq) > 1.0f)
        crossover_update_coeffs(xover, crossover_freq, sample_rate);

    float l[4], r[4], tmp[4], out[4];
    vst1q_f32(l, in_l);
    vst1q_f32(r, in_r);

    // Left — 24 dB/oct LPF (two biquads in series)
    biquad_process4(&xover->l_lpf_z1,  &xover->l_lpf_z2,  xover->lpf_coeffs, l,   tmp);
    biquad_process4(&xover->l_lpf2_z1, &xover->l_lpf2_z2, xover->lpf_coeffs, tmp, out);
    *low_l = vld1q_f32(out);

    // Left — 24 dB/oct HPF
    biquad_process4(&xover->l_hpf_z1,  &xover->l_hpf_z2,  xover->hpf_coeffs, l,   tmp);
    biquad_process4(&xover->l_hpf2_z1, &xover->l_hpf2_z2, xover->hpf_coeffs, tmp, out);
    *high_l = vld1q_f32(out);

    // Right — 24 dB/oct LPF
    biquad_process4(&xover->r_lpf_z1,  &xover->r_lpf_z2,  xover->lpf_coeffs, r,   tmp);
    biquad_process4(&xover->r_lpf2_z1, &xover->r_lpf2_z2, xover->lpf_coeffs, tmp, out);
    *low_r = vld1q_f32(out);

    // Right — 24 dB/oct HPF
    biquad_process4(&xover->r_hpf_z1,  &xover->r_hpf_z2,  xover->hpf_coeffs, r,   tmp);
    biquad_process4(&xover->r_hpf2_z1, &xover->r_hpf2_z2, xover->hpf_coeffs, tmp, out);
    *high_r = vld1q_f32(out);

    (void)mid_l; (void)mid_r;  // mid complement not currently used
}
