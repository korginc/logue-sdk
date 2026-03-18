// filters.h - SINGLE SOURCE OF TRUTH for all filter operations
#pragma once

#include <arm_neon.h>
#include <math.h>
#include "constants.h"
#include "spatial_modes.h"

// Filter constants
#define FILTER_Q_BUTTERWORTH 0.707f
#define FILTER_SAMPLE_RATE 48000.0f
#define FILTER_PI 3.141592653589793f

/**
 * Biquad filter coefficients (Transposed Direct Form II)
 */
typedef struct {
    float32x4_t b0, b1, b2, a1, a2;
} biquad_coeffs_t;

/**
 * Biquad filter state (4 parallel channels)
 */
typedef struct {
    float32x4_t z1, z2;
} biquad_state_t;

/**
 * Complete biquad filter with coeffs and state
 */
typedef struct {
    biquad_coeffs_t coeffs;
    biquad_state_t state;
} neon_biquad_t;

/**
 * Mode filter state with multiple biquads
 */
typedef struct {
    spatial_mode_t mode;
    neon_biquad_t filters[CLONE_GROUPS * 2];  // [group*2] = pre, [group*2+1] = post
    float depth_param;
    float last_depth_param;
    uint32_t ramp_samples;
} mode_filters_t;

/*===========================================================================*/
/* Biquad Filter Core Operations */
/*===========================================================================*/

fast_inline void biquad_init_state(biquad_state_t* state) {
    state->z1 = vdupq_n_f32(0.0f);
    state->z2 = vdupq_n_f32(0.0f);
}

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
/* Coefficient Calculation Functions */
/*===========================================================================*/

fast_inline void calculate_bandpass_coeffs(
    biquad_coeffs_t* coeff,
    float center_freq,
    float q_factor
) {
    float w0 = 2.0f * FILTER_PI * center_freq / FILTER_SAMPLE_RATE;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q_factor);

    float b0 = alpha;
    float b1 = 0.0f;
    float b2 = -alpha;
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

fast_inline void calculate_highpass_coeffs(
    biquad_coeffs_t* coeff,
    float cutoff_freq,
    float q_factor
) {
    float w0 = 2.0f * FILTER_PI * cutoff_freq / FILTER_SAMPLE_RATE;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q_factor);

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

fast_inline void calculate_lowpass_coeffs(
    biquad_coeffs_t* coeff,
    float cutoff_freq,
    float q_factor
) {
    float w0 = 2.0f * FILTER_PI * cutoff_freq / FILTER_SAMPLE_RATE;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q_factor);

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
/* Mode Filter Management */
/*===========================================================================*/

fast_inline void init_mode_filters(
    mode_filters_t* filters,
    spatial_mode_t mode,
    float depth
) {
    filters->mode = mode;
    filters->depth_param = depth;
    filters->last_depth_param = depth;
    filters->ramp_samples = 0;

    for (int i = 0; i < CLONE_GROUPS * 2; i++) {
        biquad_init_state(&filters->filters[i].state);
    }

    // Set initial coefficients
    update_filter_params(filters, depth, 0);
}

fast_inline void update_filter_params(
    mode_filters_t* filters,
    float depth,
    uint32_t ramp_time
) {
    filters->depth_param = depth;
    filters->ramp_samples = ramp_time;

    if (ramp_time == 0) {
        biquad_coeffs_t coeffs;

        switch (filters->mode) {
            case MODE_TRIBAL: {
                float freq = 80.0f + depth * (800.0f - 80.0f);
                calculate_bandpass_coeffs(&coeffs, freq, 2.0f);
                break;
            }
            case MODE_MILITARY: {
                float freq = 1000.0f + depth * (8000.0f - 1000.0f);
                calculate_highpass_coeffs(&coeffs, freq, 0.707f);
                break;
            }
            case MODE_ANGEL:
                // For Angel mode, we'll set in apply function
                return;
        }

        // Apply to all clone groups
        for (int g = 0; g < CLONE_GROUPS; g++) {
            filters->filters[g * 2].coeffs = coeffs;
            filters->filters[g * 2 + 1].coeffs = coeffs;
        }
    }
}

fast_inline void apply_mode_filters(
    mode_filters_t* filters,
    uint32_t group_idx,
    float32x4_t* samples_l,
    float32x4_t* samples_r,
    float depth
) {
    if (!filters) return;

    switch (filters->mode) {
        case MODE_TRIBAL:
        case MODE_MILITARY: {
            // Same filter for both channels
            *samples_l = biquad_process(*samples_l,
                &filters->filters[group_idx * 2].coeffs,
                &filters->filters[group_idx * 2].state);
            *samples_r = biquad_process(*samples_r,
                &filters->filters[group_idx * 2 + 1].coeffs,
                &filters->filters[group_idx * 2 + 1].state);
            break;
        }

        case MODE_ANGEL: {
            // Angel: HPF + LPF in series
            biquad_coeffs_t hpf_coeffs, lpf_coeffs;
            calculate_highpass_coeffs(&hpf_coeffs, 500.0f, 1.0f);
            calculate_lowpass_coeffs(&lpf_coeffs, 4000.0f + depth * 2000.0f, 1.0f);

            // HPF
            *samples_l = biquad_process(*samples_l,
                &hpf_coeffs, &filters->filters[group_idx * 2].state);
            *samples_r = biquad_process(*samples_r,
                &hpf_coeffs, &filters->filters[group_idx * 2 + 1].state);

            // LPF
            *samples_l = biquad_process(*samples_l,
                &lpf_coeffs, &filters->filters[group_idx * 2].state);
            *samples_r = biquad_process(*samples_r,
                &lpf_coeffs, &filters->filters[group_idx * 2 + 1].state);
            break;
        }
    }
}