#pragma once

/**
 * @file filters.h
 * @brief Complete filter implementations for the delay_tribal effect.
 *
 * Contains mode-specific filter types used in PercussionSpatializer.
 * Implements:
 * - Tribal mode: Bandpass filter (80-800 Hz)
 * - Military mode: Highpass filter (1 kHz+)
 * - Angel mode: Dual-band processor (HPF + LPF)
 */

#include <arm_neon.h>
#include <math.h>

// Filter constants
#define FILTER_Q_BUTTERWORTH 0.707f
#define FILTER_SAMPLE_RATE 48000.0f
#define FILTER_PI 3.141592653589793f

/**
 * Biquad filter coefficients (Transposed Direct Form II)
 * Efficient and stable for real-time audio
 */
typedef struct {
    float32x4_t b0;      // Numerator coefficients for 4 parallel filters
    float32x4_t b1;
    float32x4_t b2;
    float32x4_t a1;      // Denominator coefficients (a0 = 1.0 after normalization)
    float32x4_t a2;
} biquad_coeffs_t;

/**
 * Biquad filter state (4 parallel channels)
 */
typedef struct {
    float32x4_t z1;      // Delay element z^-1 for 4 channels
    float32x4_t z2;      // Delay element z^-2 for 4 channels
} biquad_state_t;

// mode_filters_t is defined in PercussionSpatializer.h (uses neon_biquad_t)

/*===========================================================================*/
/* Biquad Filter Core Operations */
/*===========================================================================*/

/**
 * Initialize biquad state to zero
 */
fast_inline void biquad_init_state(biquad_state_t* state) {
    state->z1 = vdupq_n_f32(0.0f);
    state->z2 = vdupq_n_f32(0.0f);
}

/**
 * Process 4 samples through 4 parallel biquad filters
 * Transposed Direct Form II - efficient and stable
 *
 * y[n] = b0*x[n] + z1[n]
 * z1[n+1] = b1*x[n] - a1*y[n] + z2[n]
 * z2[n+1] = b2*x[n] - a2*y[n]
 */
fast_inline float32x4_t biquad_process(
    float32x4_t in,
    const biquad_coeffs_t* coeff,
    biquad_state_t* state
) {
    // y = b0*x + z1
    float32x4_t y = vmlaq_f32(state->z1, in, coeff->b0);

    // Update states
    // z1 = b1*x - a1*y + z2
    state->z1 = vmlaq_f32(state->z2, in, coeff->b1);
    state->z1 = vmlsq_f32(state->z1, y, coeff->a1);

    // z2 = b2*x - a2*y
    state->z2 = vsubq_f32(vmulq_f32(in, coeff->b2), vmulq_f32(y, coeff->a2));

    return y;
}

/*===========================================================================*/
/* Coefficient Calculation - Tribal Mode (Bandpass) */
/*===========================================================================*/

/**
 * Calculate bandpass coefficients for Tribal mode
 * Center frequency: 80-800 Hz, Q = 2.0 (musical resonance)
 */
fast_inline void calculate_tribal_coeffs(
    biquad_coeffs_t* coeff,
    float center_freq,
    float q_factor
) {
    float w0 = 2.0f * FILTER_PI * center_freq / FILTER_SAMPLE_RATE;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q_factor);

    // Bandpass coefficients (un-normalized)
    float b0 = alpha;
    float b1 = 0.0f;
    float b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    // Normalize by a0
    float inv_a0 = 1.0f / a0;
    float32x4_t b0_vec = vdupq_n_f32(b0 * inv_a0);
    float32x4_t b1_vec = vdupq_n_f32(b1 * inv_a0);
    float32x4_t b2_vec = vdupq_n_f32(b2 * inv_a0);
    float32x4_t a1_vec = vdupq_n_f32(a1 * inv_a0);
    float32x4_t a2_vec = vdupq_n_f32(a2 * inv_a0);

    // Store coefficients (same for all 4 parallel filters)
    coeff->b0 = b0_vec;
    coeff->b1 = b1_vec;
    coeff->b2 = b2_vec;
    coeff->a1 = a1_vec;
    coeff->a2 = a2_vec;
}

/*===========================================================================*/
/* Coefficient Calculation - Military Mode (Highpass) */
/*===========================================================================*/

/**
 * Calculate highpass coefficients for Military mode
 * Cutoff: 1 kHz+, Q = 0.707 (Butterworth response)
 */
fast_inline void calculate_military_coeffs(
    biquad_coeffs_t* coeff,
    float cutoff_freq,
    float q_factor
) {
    float w0 = 2.0f * FILTER_PI * cutoff_freq / FILTER_SAMPLE_RATE;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q_factor);

    // Highpass coefficients
    float b0 = (1.0f + cos_w0) / 2.0f;
    float b1 = -(1.0f + cos_w0);
    float b2 = b0;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    float inv_a0 = 1.0f / a0;
    coeff->b0 = vdupq_n_f32(b0 * inv_a0);
    coeff->b1 = vdupq_n_f32(b1 * inv_a0);
    coeff->b2 = vdupq_n_f32(b2 * inv_a0);
    coeff->a1 = vdupq_n_f32(a1 * inv_a0);
    coeff->a2 = vdupq_n_f32(a2 * inv_a0);
}

/*===========================================================================*/
/* Coefficient Calculation - Angel Mode (Dual-band) */
/*===========================================================================*/

/**
 * Calculate lowpass coefficients for Angel mode
 * Used for the ethereal smoothing
 */
fast_inline void calculate_lowpass_coeffs(
    biquad_coeffs_t* coeff,
    float cutoff_freq,
    float q_factor
) {
    float w0 = 2.0f * FILTER_PI * cutoff_freq / FILTER_SAMPLE_RATE;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q_factor);

    // Lowpass coefficients
    float b0 = (1.0f - cos_w0) / 2.0f;
    float b1 = 1.0f - cos_w0;
    float b2 = b0;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w0;
    float a2 = 1.0f - alpha;

    float inv_a0 = 1.0f / a0;
    coeff->b0 = vdupq_n_f32(b0 * inv_a0);
    coeff->b1 = vdupq_n_f32(b1 * inv_a0);
    coeff->b2 = vdupq_n_f32(b2 * inv_a0);
    coeff->a1 = vdupq_n_f32(a1 * inv_a0);
    coeff->a2 = vdupq_n_f32(a2 * inv_a0);
}

/*===========================================================================*/
/* Mode Filter Initialization */
/*===========================================================================*/

/**
 * Initialize Tribal mode filters
 * Bandpass: 80-800 Hz center, Q=2.0
 */
fast_inline void init_tribal_filters(
    mode_filters_t* filters,
    float center_freq,
    float q_factor
) {
    filters->mode = 0;
    filters->center_freq = center_freq;
    filters->q_factor = q_factor;

    // Calculate coefficients
    calculate_tribal_coeffs(&filters->pre_coeff, center_freq, q_factor);

    // Copy to post filter (same coefficients)
    filters->post_coeff = filters->pre_coeff;

    // Initialize states
    for (int i = 0; i < MAX_CLONE_GROUPS; i++) {
        biquad_init_state(&filters->pre_state[i]);
        biquad_init_state(&filters->post_state[i]);
    }
}

/**
 * Initialize Military mode filters
 * Highpass: 1 kHz+ cutoff, Q=0.707 (Butterworth)
 */
fast_inline void init_military_filters(
    mode_filters_t* filters,
    float cutoff_freq,
    float q_factor
) {
    filters->mode = 1;
    filters->center_freq = cutoff_freq;
    filters->q_factor = q_factor;

    calculate_military_coeffs(&filters->pre_coeff, cutoff_freq, q_factor);
    filters->post_coeff = filters->pre_coeff;

    for (int i = 0; i < MAX_CLONE_GROUPS; i++) {
        biquad_init_state(&filters->pre_state[i]);
        biquad_init_state(&filters->post_state[i]);
    }
}

/**
 * Initialize Angel mode filters
 * Dual-band: HPF (500 Hz) + gentle LPF (4 kHz)
 */
fast_inline void init_angel_filters(
    mode_filters_t* filters,
    float low_cut,
    float high_cut,
    float q_factor
) {
    filters->mode = 2;
    filters->center_freq = (low_cut + high_cut) * 0.5f;
    filters->q_factor = q_factor;

    // Pre-filter: Highpass (remove rumble)
    calculate_military_coeffs(&filters->pre_coeff, low_cut, 0.707f);

    // Post-filter: Lowpass (smooth ethereal character)
    calculate_lowpass_coeffs(&filters->post_coeff, high_cut, q_factor);

    for (int i = 0; i < MAX_CLONE_GROUPS; i++) {
        biquad_init_state(&filters->pre_state[i]);
        biquad_init_state(&filters->post_state[i]);
    }
}

/*===========================================================================*/
/* Main Filter Application Function */
/*===========================================================================*/

/**
 * Apply mode-specific filters to a clone group
 * @param filters Filter state (per mode)
 * @param group_idx Which clone group (0-3)
 * @param samples_l Left channel samples (4 clones)
 * @param samples_r Right channel samples (4 clones)
 */
fast_inline void apply_mode_filters(
    mode_filters_t* filters,
    uint32_t group_idx,
    float32x4_t* samples_l,
    float32x4_t* samples_r
) {
    // Apply pre-filter
    if (group_idx < MAX_CLONE_GROUPS) {
        *samples_l = biquad_process(*samples_l,
                                     &filters->pre_coeff,
                                     &filters->pre_state[group_idx]);
        *samples_r = biquad_process(*samples_r,
                                     &filters->pre_coeff,
                                     &filters->pre_state[group_idx]);

        // Apply post-filter
        *samples_l = biquad_process(*samples_l,
                                     &filters->post_coeff,
                                     &filters->post_state[group_idx]);
        *samples_r = biquad_process(*samples_r,
                                     &filters->post_coeff,
                                     &filters->post_state[group_idx]);
    }
}

/**
 * Update filter coefficients when parameters change
 * (e.g., when Depth or Rate parameters affect filter response)
 */
fast_inline void update_filter_params(
    mode_filters_t* filters,
    float new_freq,
    float new_q
) {
    switch (filters->mode) {
        case 0: // Tribal
            calculate_tribal_coeffs(&filters->pre_coeff, new_freq, new_q);
            filters->post_coeff = filters->pre_coeff;
            break;
        case 1: // Military
            calculate_military_coeffs(&filters->pre_coeff, new_freq, new_q);
            filters->post_coeff = filters->pre_coeff;
            break;
        case 2: // Angel
            // For Angel, new_freq affects the high cut
            calculate_lowpass_coeffs(&filters->post_coeff, new_freq, new_q);
            break;
    }
    filters->center_freq = new_freq;
    filters->q_factor = new_q;
}