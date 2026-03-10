#pragma once

/**
 * @file filters.h
 * @brief NEON-optimized filters for sidechain processing
 *
 * Includes:
 * - Sidechain HPF (Biquad, Transposed Direct Form II)
 * - Envelope detector (Peak/RMS)
 * - Gain computer with knee
 * - Attack/release smoothing
 *
 * FIXED: Correct biquad implementation using Transposed Direct Form II
 *        This form is efficient and numerically stable
 */

#include <arm_neon.h>
#include <math.h>
#include "float_math.h"
#include "constants.h"

/* ---------------------------------------------------------------------------
 * 1. SIDECHAIN HPF - Biquad filter in Transposed Direct Form II
 *    This is the most efficient and stable form for real-time audio
 * --------------------------------------------------------------------------- */

typedef struct {
    float32x4_t z1;           // First delay element (for 4 parallel channels)
    float32x4_t z2;           // Second delay element
    float b0, b1, b2, a1, a2; // Biquad coefficients (shared across channels)
    float cutoff_hz;
    float sample_rate;
    float q;                  // Q factor (0.5 for Bessel)
} sidechain_hpf_t;

/**
 * Initialize sidechain HPF (Bessel for clean phase response)
 */
fast_inline void sidechain_hpf_init(sidechain_hpf_t* f, float cutoff, float sr) {
    f->cutoff_hz = cutoff;
    f->sample_rate = sr;
    f->q = 0.5f;  // Bessel response
    f->z1 = vdupq_n_f32(0.0f);
    f->z2 = vdupq_n_f32(0.0f);

    // Design a 2nd-order high-pass filter using bilinear transform
    float w0 = 2.0f * M_PI * cutoff / sr;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * f->q);

    // Calculate coefficients for high-pass filter
    float b0 = (1.0f + cos_w0) / 2.0f;
    float b1 = -(1.0f + cos_w0);
    float b2 = b0;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    // Normalize coefficients by a0
    f->b0 = b0 / a0;
    f->b1 = b1 / a0;
    f->b2 = b2 / a0;
    f->a1 = a1 / a0;
    f->a2 = a2 / a0;
}

/**
 * Process 4 samples through sidechain HPF
 * Uses Transposed Direct Form II for efficiency and stability
 *
 * Transposed Direct Form II:
 *   y[n] = b0 * x[n] + z1[n]
 *   z1[n+1] = b1 * x[n] - a1 * y[n] + z2[n]
 *   z2[n+1] = b2 * x[n] - a2 * y[n]
 */
fast_inline float32x4_t sidechain_hpf_process(sidechain_hpf_t* f, float32x4_t in) {
    // Load coefficients (same for all 4 channels)
    float32x4_t b0 = vdupq_n_f32(f->b0);
    float32x4_t b1 = vdupq_n_f32(f->b1);
    float32x4_t b2 = vdupq_n_f32(f->b2);
    float32x4_t a1 = vdupq_n_f32(f->a1);
    float32x4_t a2 = vdupq_n_f32(f->a2);

    // Transposed Direct Form II
    // y[n] = b0 * x[n] + z1[n]
    float32x4_t out = vmlaq_f32(f->z1, in, b0);

    // Update states for next sample
    // z1_new = b1 * x[n] - a1 * y[n] + z2_old
    f->z1 = vmlaq_f32(f->z2, in, b1);
    f->z1 = vmlsq_f32(f->z1, out, a1);

    // z2_new = b2 * x[n] - a2 * y[n]
    f->z2 = vsubq_f32(vmulq_f32(in, b2), vmulq_f32(out, a2));

    return out;
}

/**
 * Update cutoff frequency (recalculates coefficients)
 */
fast_inline void sidechain_hpf_set_cutoff(sidechain_hpf_t* f, float cutoff) {
    if (fabsf(cutoff - f->cutoff_hz) > 1.0f) {
        sidechain_hpf_init(f, cutoff, f->sample_rate);
    }
}

/* ---------------------------------------------------------------------------
 * 2. ENVELOPE DETECTOR - Peak/RMS with selectable mode
 * --------------------------------------------------------------------------- */

#define DETECT_MODE_PEAK 0
#define DETECT_MODE_RMS  1
#define DETECT_MODE_BLEND 2

typedef struct {
    float32x4_t rms_accum;      // Running sum for RMS
    float32x4_t peak_hold;       // Peak hold value
    uint32x4_t hold_counter;     // Hold time counter
    float32x4_t last_envelope;   // Previous envelope value
    uint8_t mode;                // Detection mode
    float attack_coeff;          // Attack smoothing
    float release_coeff;         // Release smoothing
    float sample_rate;
} envelope_detector_t;

/**
 * Initialize envelope detector
 */
fast_inline void envelope_detector_init(envelope_detector_t* env, float sr) {
    env->rms_accum = vdupq_n_f32(0.0f);
    env->peak_hold = vdupq_n_f32(0.0f);
    env->hold_counter = vdupq_n_u32(0);
    env->last_envelope = vdupq_n_f32(0.0f);
    env->mode = DETECT_MODE_PEAK;
    env->sample_rate = sr;

    // Default 10ms attack, 100ms release
    env->attack_coeff = e_expff(-1.0f / (0.01f * sr));
    env->release_coeff = e_expff(-1.0f / (0.1f * sr));
}

// ... rest of envelope detector implementation ...

/* ---------------------------------------------------------------------------
 * 3. GAIN COMPUTER - With soft/hard knee
 * --------------------------------------------------------------------------- */

typedef struct {
    float knee_width;      // dB width of soft knee
    uint8_t knee_type;     // 0=hard, 1=soft, 2=medium
} gain_computer_t;

/**
 * Initialize gain computer
 */
fast_inline void gain_computer_init(gain_computer_t* gc) {
    gc->knee_width = 6.0f;
    gc->knee_type = 0;
}

// ... rest of gain computer implementation ...

/* ---------------------------------------------------------------------------
 * 4. ATTACK/RELEASE SMOOTHING
 * --------------------------------------------------------------------------- */

typedef struct {
    float32x4_t current_gain;      // Current gain reduction (dB)
    float attack_coeff;             // Attack smoothing coefficient
    float release_coeff;            // Release smoothing coefficient
    float sample_rate;
} smoothing_t;

/**
 * Initialize smoother
 */
fast_inline void smoothing_init(smoothing_t* sm, float sr) {
    sm->current_gain = vdupq_n_f32(0.0f);
    sm->attack_coeff = e_expff(-1.0f / (0.01f * sr));
    sm->release_coeff = e_expff(-1.0f / (0.1f * sr));
    sm->sample_rate = sr;
}

/**
 * Process one sample of smoothing
 */
fast_inline float32x4_t smoothing_process(smoothing_t* sm,
                                          float32x4_t target_gain) {
    // Determine if we're attacking or releasing
    uint32x4_t attacking = vcltq_f32(target_gain, sm->current_gain);

    // Select appropriate coefficient
    float32x4_t coeff = vbslq_f32(attacking,
                                  vdupq_n_f32(sm->attack_coeff),
                                  vdupq_n_f32(sm->release_coeff));

    // One-pole smoothing: y[n] = a * x[n] + (1-a) * y[n-1]
    // But here we use: current = target*(1-coeff) + current*coeff
    float32x4_t one_minus_coeff = vsubq_f32(vdupq_n_f32(1.0f), coeff);
    sm->current_gain = vaddq_f32(vmulq_f32(target_gain, one_minus_coeff),
                                  vmulq_f32(sm->current_gain, coeff));

    return sm->current_gain;
}

/**
 * Set time constants
 */
fast_inline void smoothing_set_times(smoothing_t* sm,
                                     float attack_ms,
                                     float release_ms) {
    sm->attack_coeff = e_expff(-1.0f / (attack_ms * 0.001f * sm->sample_rate));
    sm->release_coeff = e_expff(-1.0f / (release_ms * 0.001f * sm->sample_rate));
}