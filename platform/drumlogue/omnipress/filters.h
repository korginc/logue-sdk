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
 * Complete filter suite for OmniPress
 * Includes: Sidechain HPF, Envelope followers, Smoothing filters
 */

#include <arm_neon.h>
#include <math.h>

/* ---------------------------------------------------------------------------
 * 1. SIDECHAIN HPF - 12dB/oct Bessel for clean sidechain
 * --------------------------------------------------------------------------- */

typedef struct {
    float32x4_t z1;           // Delay elements for 4 parallel channels
    float32x4_t z2;
    float b0, b1, b2, a1, a2; // Biquad coefficients (shared)
    float cutoff_hz;
    float sample_rate;
} sidechain_hpf_t;

/**
 * Initialize sidechain HPF (Bessel for clean phase response)
 */
fast_inline void sidechain_hpf_init(sidechain_hpf_t* f, float cutoff, float sr) {
    f->cutoff_hz = cutoff;
    f->sample_rate = sr;
    f->z1 = vdupq_n_f32(0.0f);
    f->z2 = vdupq_n_f32(0.0f);

    // Pre-warp for bilinear transform
    float w0 = 2.0f * M_PI * cutoff / sr;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float Q = 0.5f;  // Bessel Q

    // Calculate alpha
    float alpha = sin_w0 / (2.0f * Q);

    // Biquad coefficients (normalized)
    f->b0 = (1.0f + cos_w0) / 2.0f;
    f->b1 = -(1.0f + cos_w0);
    f->b2 = f->b0;
    f->a1 = -2.0f * cos_w0;
    f->a2 = 1.0f - alpha;

    // Normalize by a0 = 1 + alpha
    float a0 = 1.0f + alpha;
    f->b0 /= a0;
    f->b1 /= a0;
    f->b2 /= a0;
    f->a1 /= a0;
    f->a2 /= a0;
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

    // Direct Form I
    float32x4_t out = vaddq_f32(vmlaq_f32(f->z1, in, b0), vmulq_f32(f->z2, b1));

    // Update states for next sample
    // z1_new = b1 * x[n] - a1 * y[n] + z2_old
    f->z2 = vaddq_f32(vmulq_f32(in, b2), vmlsq_f32(f->z1, out, a1));

    // z2_new = b2 * x[n] - a2 * y[n]
    f->z1 = vaddq_f32(vmulq_f32(in, b1), vmlsq_f32(vdupq_n_f32(0.0f), out, a2));

    return out;
}

/**
 * Update cutoff frequency (recalculates coefficients for smooth transition)
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
    env->attack_coeff = expf(-1.0f / (0.01f * sr));
    env->release_coeff = expf(-1.0f / (0.1f * sr));
}

// Set attack/release times
fast_inline void envelope_set_attack_release(envelope_detector_t* env,
                                             float attack_ms,
                                             float release_ms) {
    env->attack_coeff = expf(-1.0f / (attack_ms * 0.001f * env->sample_rate));
    env->release_coeff = expf(-1.0f / (release_ms * 0.001f * env->sample_rate));
}

// Process 4 samples through envelope detector
fast_inline float32x4_t envelope_detect(envelope_detector_t* env,
                                        float32x4_t in,
                                        float32x4_t sidechain) {
    float32x4_t abs_in = vabsq_f32(sidechain);
    float32x4_t envelope;

    switch (env->mode) {
        case DETECT_MODE_PEAK: {
            // Peak detection with hold
            uint32x4_t new_peak = vcgtq_f32(abs_in, env->peak_hold);
            env->peak_hold = vbslq_f32(new_peak, abs_in, env->peak_hold);

            // Hold counter
            env->hold_counter = vaddq_u32(env->hold_counter, vdupq_n_u32(1));
            uint32x4_t hold_done = vcgtq_u32(env->hold_counter, vdupq_n_u32(5000)); // 10ms hold

            // Decay peak when hold expires
            env->peak_hold = vbslq_f32(hold_done,
                                       vmulq_f32(env->peak_hold, vdupq_n_f32(0.999f)),
                                       env->peak_hold);
            env->hold_counter = vbslq_u32(hold_done, vdupq_n_u32(0), env->hold_counter);

            envelope = env->peak_hold;
            break;
        }

        case DETECT_MODE_RMS: {
            // RMS with 50ms window
            float32x4_t squared = vmulq_f32(sidechain, sidechain);
            float alpha = 0.01f;  // 10ms time constant
            env->rms_accum = vaddq_f32(vmulq_f32(squared, vdupq_n_f32(alpha)),
                                       vmulq_f32(env->rms_accum, vdupq_n_f32(1.0f - alpha)));

            // Approximate square root
            envelope = vrsqrteq_f32(env->rms_accum);  // 1/sqrt(x)
            envelope = vmulq_f32(env->rms_accum, envelope);  // x * 1/sqrt(x) = sqrt(x)
            break;
        }

        case DETECT_MODE_BLEND: {
            // Blend peak and RMS (like SSL console)
            float32x4_t peak = env->peak_hold;

            float32x4_t squared = vmulq_f32(sidechain, sidechain);
            float alpha = 0.01f;
            env->rms_accum = vaddq_f32(vmulq_f32(squared, vdupq_n_f32(alpha)),
                                       vmulq_f32(env->rms_accum, vdupq_n_f32(1.0f - alpha)));
            float32x4_t rms = vmulq_f32(env->rms_accum, vrsqrteq_f32(env->rms_accum));

            envelope = vaddq_f32(vmulq_f32(peak, vdupq_n_f32(0.7f)),
                                 vmulq_f32(rms, vdupq_n_f32(0.3f)));
            break;
        }

        default:
            envelope = abs_in;
    }

    // Smooth with attack/release
    uint32x4_t attacking = vcgtq_f32(envelope, env->last_envelope);
    float32x4_t coeff = vbslq_f32(attacking,
                                  vdupq_n_f32(env->attack_coeff),
                                  vdupq_n_f32(env->release_coeff));

    env->last_envelope = vaddq_f32(vmulq_f32(envelope, vsubq_f32(vdupq_n_f32(1.0f), coeff)),
                                   vmulq_f32(env->last_envelope, coeff));

    return env->last_envelope;
}

/* ---------------------------------------------------------------------------
 * 3. GAIN COMPUTER - With soft/hard knee and ratio logic
 * --------------------------------------------------------------------------- */

#define KNEE_HARD 0
#define KNEE_SOFT 1
#define KNEE_MEDIUM 2

typedef struct {
    float knee_width;      // dB width of soft knee
    uint8_t knee_type;     // 0=hard, 1=soft, 2=medium
} gain_computer_t;

/**
 * Initialize gain computer
 */
fast_inline void gain_computer_init(gain_computer_t* gc) {
    gc->knee_width = 6.0f;  // 6dB soft knee
    gc->knee_type = KNEE_MEDIUM;
}

// Convert linear to dB (approximation)
fast_inline float32x4_t linear_to_db(float32x4_t linear) {
    // log10(x) ≈ log2(x) * 0.30103
    uint32x4_t u = vreinterpretq_u32_f32(linear);
    uint32x4_t exp = vandq_u32(u, vdupq_n_u32(0x7F800000));
    uint32x4_t mant = vandq_u32(u, vdupq_n_u32(0x007FFFFF));

    float32x4_t exp_f = vcvtq_f32_u32(vshrq_n_u32(exp, 23));
    float32x4_t mant_f = vcvtq_f32_u32(mant);
    float32x4_t log2 = vaddq_f32(vsubq_f32(exp_f, vdupq_n_f32(127.0f)),
                                  vmulq_f32(mant_f, vdupq_n_f32(1.0f / (1 << 23))));

    return vmulq_f32(log2, vdupq_n_f32(6.0206f));  // 20 * log10(2)
}

// Compute gain reduction with knee
fast_inline float32x4_t gain_computer_process(gain_computer_t* gc,
                                              float32x4_t envelope_db,
                                              float thresh_db,
                                              float ratio) {
    float32x4_t thresh = vdupq_n_f32(thresh_db);
    float32x4_t ratio_v = vdupq_n_f32(ratio);
    float32x4_t one = vdupq_n_f32(1.0f);

    // Calculate overshoot
    float32x4_t overshoot = vsubq_f32(envelope_db, thresh);

    // Apply knee based on type
    float32x4_t gain_red;

    switch (gc->knee_type) {
        case KNEE_HARD: {
            // Hard knee - instantaneous
            uint32x4_t above = vcgtq_f32(envelope_db, thresh);
            gain_red = vbslq_f32(above,
                                 vmulq_f32(overshoot, vsubq_f32(one, vrecpeq_f32(ratio_v))),
                                 vdupq_n_f32(0.0f));
            break;
        }

        case KNEE_SOFT: {
            // Soft knee - polynomial transition
            float32x4_t knee_w = vdupq_n_f32(gc->knee_width);
            float32x4_t knee_start = vsubq_f32(thresh, vmulq_f32(knee_w, vdupq_n_f32(0.5f)));
            float32x4_t knee_end = vaddq_f32(thresh, vmulq_f32(knee_w, vdupq_n_f32(0.5f)));

            // In knee region
            uint32x4_t in_knee = vandq_u32(vcgtq_f32(envelope_db, knee_start),
                                           vcltq_f32(envelope_db, knee_end));

            // Above knee
            uint32x4_t above_knee = vcgtq_f32(envelope_db, knee_end);

            // Calculate gain reduction for knee region (quadratic)
            float32x4_t x = vdivq_f32(vsubq_f32(envelope_db, knee_start), knee_w);
            float32x4_t knee_gr = vmulq_f32(vmulq_f32(x, x), vdupq_n_f32(0.5f));
            knee_gr = vmulq_f32(knee_gr, vsubq_f32(one, vrecpeq_f32(ratio_v)));

            gain_red = vbslq_f32(in_knee, knee_gr,
                        vbslq_f32(above_knee,
                                 vmulq_f32(overshoot, vsubq_f32(one, vrecpeq_f32(ratio_v))),
                                 vdupq_n_f32(0.0f)));
            break;
        }

        case KNEE_MEDIUM:
        default: {
            // Medium knee - linear transition
            float32x4_t knee_w = vdupq_n_f32(gc->knee_width);
            float32x4_t knee_start = vsubq_f32(thresh, vmulq_f32(knee_w, vdupq_n_f32(0.5f)));

            // Linear interpolation in knee region
            float32x4_t slope = vsubq_f32(one, vrecpeq_f32(ratio_v));
            float32x4_t in_knee_amt = vdivq_f32(vsubq_f32(envelope_db, knee_start), knee_w);
            in_knee_amt = vmaxq_f32(vminq_f32(in_knee_amt, one), vdupq_n_f32(0.0f));

            gain_red = vmulq_f32(vmulq_f32(overshoot, slope), in_knee_amt);
            break;
        }
    }

    return vnegq_f32(gain_red);  // Return negative dB (gain reduction)
}

/* ---------------------------------------------------------------------------
 * 4. ATTACK/RELEASE SMOOTHING - With auto time constants
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
    sm->attack_coeff = expf(-1.0f / (0.01f * sr));    // 10ms default
    sm->release_coeff = expf(-1.0f / (0.1f * sr));    // 100ms default
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
    sm->current_gain = vaddq_f32(vmulq_f32(target_gain, vsubq_f32(vdupq_n_f32(1.0f), coeff)),
                                 vmulq_f32(sm->current_gain, coeff));

    return sm->current_gain;
}

/**
 * Set time constants
 */
fast_inline void smoothing_set_times(smoothing_t* sm,
                                     float attack_ms,
                                     float release_ms) {
    sm->attack_coeff = expf(-1.0f / (attack_ms * 0.001f * sm->sample_rate));
    sm->release_coeff = expf(-1.0f / (release_ms * 0.001f * sm->sample_rate));
}
