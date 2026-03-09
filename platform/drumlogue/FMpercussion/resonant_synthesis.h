#pragma once

/**
 * @file resonant_synthesis.h
 * @brief Resonant synthesis engine with morph control
 *
 * Based on Lazzarini (2017) Section 4.10.3
 * Now with 5 resonant modes and morph parameter for expressive control
 */

#include <arm_neon.h>
#include <math.h>
#include "constants.h"
#include "sine_neon.h"

/**
 * Resonant mode types
 */
typedef enum {
    RESONANT_MODE_LOWPASS = 0,   // Single-sided (low-pass character)
    RESONANT_MODE_BANDPASS = 1,  // Double-sided (band-pass character)
    RESONANT_MODE_HIGHPASS = 2,  // Derived high-pass
    RESONANT_MODE_NOTCH = 3,     // Notch filter character
    RESONANT_MODE_PEAK = 4       // Peak filter (boost around center)
} resonant_mode_t;

/**
 * Resonant synthesis engine state
 */
typedef struct {
    // Phase accumulators (4 voices)
    float32x4_t phase_f0;      // Fundamental phase
    float32x4_t phase_fc;      // Center frequency phase

    // Base frequencies
    float32x4_t f0;            // Fundamental frequencies (from MIDI)
    float32x4_t fc;            // Center frequencies (resonance peak)

    // Parameters
    float32x4_t resonance;     // 'a' parameter from paper (0-0.99)
    float32x4_t mix;           // Blend between single/double-sided (0-1)
    float32x4_t gain;          // Output gain (for peak mode)
    resonant_mode_t mode;      // Current resonant mode

    // Morph control (0-1)
    float32x4_t morph;         // Single-knob control affecting multiple parameters

    // Parameter smoothing
    float32x4_t target_fc;
    float32x4_t target_resonance;
    uint32x4_t ramp_counter;

    // Constants
    float32x4_t one;
    float32x4_t two;
    float32x4_t epsilon;
} resonant_synth_t;

/**
 * Initialize resonant synthesis engine
 */
fast_inline void resonant_synth_init(resonant_synth_t* rs) {
    rs->phase_f0 = vdupq_n_f32(0.0f);
    rs->phase_fc = vdupq_n_f32(0.0f);

    rs->f0 = vdupq_n_f32(200.0f);      // Default mid tom
    rs->fc = vdupq_n_f32(1000.0f);     // Default center

    rs->resonance = vdupq_n_f32(0.5f);
    rs->mix = vdupq_n_f32(0.5f);
    rs->gain = vdupq_n_f32(1.0f);
    rs->mode = RESONANT_MODE_BANDPASS;
    rs->morph = vdupq_n_f32(0.5f);

    rs->target_fc = vdupq_n_f32(1000.0f);
    rs->target_resonance = vdupq_n_f32(0.5f);
    rs->ramp_counter = vdupq_n_u32(0);

    rs->one = vdupq_n_f32(1.0f);
    rs->two = vdupq_n_f32(2.0f);
    rs->epsilon = vdupq_n_f32(RESONANT_DENOM_EPSILON);
}

/**
 * Set fundamental frequency (from MIDI note)
 */
fast_inline void resonant_synth_set_f0(resonant_synth_t* rs,
                                       uint32x4_t voice_mask,
                                       float32x4_t midi_notes) {
    // Convert MIDI to frequency using fast approximation
    float32x4_t a4_freq = vdupq_n_f32(A4_FREQ);
    float32x4_t a4_midi = vdupq_n_f32(A4_MIDI);
    float32x4_t twelfth = vdupq_n_f32(1.0f/12.0f);

    float32x4_t exponent = vmulq_f32(vsubq_f32(midi_notes, a4_midi), twelfth);

    // 2^x approximation: 1 + x*0.693 + x²*0.24
    float32x4_t x2 = vmulq_f32(exponent, exponent);
    float32x4_t two_pow = vmlaq_f32(vdupq_n_f32(1.0f), exponent, vdupq_n_f32(0.693f));
    two_pow = vmlaq_f32(two_pow, x2, vdupq_n_f32(0.24f));

    float32x4_t base_freq = vmulq_f32(a4_freq, two_pow);

    rs->f0 = vbslq_f32(vreinterpretq_f32_u32(voice_mask),
                        base_freq, rs->f0);
}

/**
 * Set center frequency with smoothing
 */
fast_inline void resonant_synth_set_center(resonant_synth_t* rs,
                                           uint32x4_t voice_mask,
                                           float32x4_t fc_hz) {
    rs->target_fc = vbslq_f32(vreinterpretq_f32_u32(voice_mask),
                               fc_hz, rs->target_fc);

    // Start ramp for selected voices (1ms @48kHz = 48 samples)
    rs->ramp_counter = vbslq_u32(voice_mask,
                                  vdupq_n_u32(48),
                                  rs->ramp_counter);
}

/**
 * Set resonance amount with smoothing
 */
fast_inline void resonant_synth_set_resonance(resonant_synth_t* rs,
                                              uint32x4_t voice_mask,
                                              float resonance_percent) {
    // Map 0-100% to 0-0.99 (a must be < 1)
    float r = (resonance_percent / 100.0f) * 0.99f;
    rs->target_resonance = vbslq_f32(vreinterpretq_f32_u32(voice_mask),
                                      vdupq_n_f32(r),
                                      rs->target_resonance);

    rs->ramp_counter = vbslq_u32(voice_mask,
                                  vdupq_n_u32(48),
                                  rs->ramp_counter);
}

/**
 * Set resonant mode
 */
fast_inline void resonant_synth_set_mode(resonant_synth_t* rs,
                                         uint32x4_t voice_mask,
                                         resonant_mode_t mode) {
    rs->mode = mode;
}

/**
 * Apply morph parameter - controls multiple dimensions based on mode
 * @param morph 0-1
 */
fast_inline void resonant_synth_apply_morph(resonant_synth_t* rs,
                                            uint32x4_t voice_mask,
                                            float morph) {
    rs->morph = vdupq_n_f32(morph);

    float fc, res, gain;

    switch (rs->mode) {
        case RESONANT_MODE_LOWPASS:
            // LowPass: morph controls cutoff frequency
            fc = 50.0f + morph * 7950.0f;  // 50-8000 Hz
            res = 0.5f;                     // Fixed resonance
            gain = 1.0f;
            break;

        case RESONANT_MODE_BANDPASS:
            // BandPass: morph controls Q/resonance
            fc = 1000.0f;                    // Fixed center
            res = 0.1f + morph * 0.8f;        // 0.1-0.9
            gain = 1.0f;
            break;

        case RESONANT_MODE_HIGHPASS:
            // HighPass: morph controls cutoff (inverse)
            fc = 8000.0f - morph * 7950.0f;  // 8000-50 Hz
            res = 0.3f;                       // Fixed resonance
            gain = 1.0f;
            break;

        case RESONANT_MODE_NOTCH:
            // Notch: morph controls notch width
            fc = 1000.0f;                    // Fixed center
            res = 0.2f + morph * 0.7f;        // 0.2-0.9
            gain = 1.0f;
            break;

        case RESONANT_MODE_PEAK:
            // Peak: morph controls both frequency and gain
            fc = 200.0f + morph * 3800.0f;   // 200-4000 Hz
            res = 0.3f + morph * 0.6f;        // 0.3-0.9
            gain = 1.0f + morph * 3.0f;       // 1-4x
            break;

        default:
            fc = 1000.0f;
            res = 0.5f;
            gain = 1.0f;
    }

    // Apply parameters
    resonant_synth_set_center(rs, voice_mask, vdupq_n_f32(fc));
    resonant_synth_set_resonance(rs, voice_mask, res * 100.0f);
    rs->gain = vdupq_n_f32(gain);
}

/**
 * Process one sample of resonant synthesis
 * @return Output sample for all 4 voices
 */
fast_inline float32x4_t resonant_synth_process(resonant_synth_t* rs,
                                               uint32x4_t voice_mask) {
    // =================================================================
    // 1. Smooth parameters (prevent zipper noise)
    // =================================================================
    uint32x4_t ramping = vcgtq_u32(rs->ramp_counter, vdupq_n_u32(0));

    // Smooth resonance
    float32x4_t res_step = vsubq_f32(rs->target_resonance, rs->resonance);
    res_step = vmulq_f32(res_step, vdupq_n_f32(1.0f/48.0f));

    rs->resonance = vbslq_f32(vreinterpretq_f32_u32(ramping),
                               vaddq_f32(rs->resonance, res_step),
                               rs->resonance);

    // Smooth center frequency
    float32x4_t fc_step = vsubq_f32(rs->target_fc, rs->fc);
    fc_step = vmulq_f32(fc_step, vdupq_n_f32(1.0f/48.0f));

    rs->fc = vbslq_f32(vreinterpretq_f32_u32(ramping),
                        vaddq_f32(rs->fc, fc_step),
                        rs->fc);

    // Decrement ramp counters
    rs->ramp_counter = vbslq_u32(ramping,
                                  vsubq_u32(rs->ramp_counter, vdupq_n_u32(1)),
                                  rs->ramp_counter);

    // Snap to target when done
    uint32x4_t done = vceqq_u32(rs->ramp_counter, vdupq_n_u32(0));
    rs->resonance = vbslq_f32(vreinterpretq_f32_u32(done),
                               rs->target_resonance,
                               rs->resonance);
    rs->fc = vbslq_f32(vreinterpretq_f32_u32(done),
                        rs->target_fc,
                        rs->fc);

    // =================================================================
    // 2. Update phases
    // =================================================================
    float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI * INV_SAMPLE_RATE);

    float32x4_t f0_inc = vmulq_f32(rs->f0, two_pi_over_sr);
    float32x4_t fc_inc = vmulq_f32(rs->fc, two_pi_over_sr);

    rs->phase_f0 = vaddq_f32(rs->phase_f0, f0_inc);
    rs->phase_fc = vaddq_f32(rs->phase_fc, fc_inc);

    // Wrap phases to [0, 2π)
    float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);
    uint32x4_t wrap_f0 = vcgeq_f32(rs->phase_f0, two_pi);
    uint32x4_t wrap_fc = vcgeq_f32(rs->phase_fc, two_pi);

    rs->phase_f0 = vbslq_f32(wrap_f0,
                              vsubq_f32(rs->phase_f0, two_pi),
                              rs->phase_f0);
    rs->phase_fc = vbslq_f32(wrap_fc,
                              vsubq_f32(rs->phase_fc, two_pi),
                              rs->phase_fc);

    // =================================================================
    // 3. Generate carrier and modulator signals
    // =================================================================
    float32x4_t sin_f0 = neon_sin(rs->phase_f0);
    float32x4_t cos_fc = neon_cos(rs->phase_fc);

    // =================================================================
    // 4. Apply summation formulae
    // =================================================================
    // Denominator: 1 - 2a cos(θ) + a²
    float32x4_t a = rs->resonance;
    float32x4_t a_sq = vmulq_f32(a, a);
    float32x4_t two_a = vmulq_f32(rs->two, a);

    float32x4_t denom = vsubq_f32(vaddq_f32(rs->one, a_sq),
                                   vmulq_f32(two_a, cos_fc));

    // Protect against division by zero
    denom = vmaxq_f32(denom, rs->epsilon);

    // Single-sided (low-pass) component: s(t) = sin(ωt) / (1 - 2a cos(θ) + a²)
    float32x4_t low_pass = vdivq_f32(sin_f0, denom);

    // Double-sided (band-pass) component: s(t) = (1 - a²) sin(ωt) / (1 - 2a cos(θ) + a²)
    float32x4_t band_pass = vdivq_f32(vmulq_f32(vsubq_f32(rs->one, a_sq), sin_f0), denom);

    // =================================================================
    // 5. Apply mode-specific transformations
    // =================================================================
    float32x4_t output;
    float32x4_t scale = vdupq_n_f32(1.0f);

    switch (rs->mode) {
        case RESONANT_MODE_LOWPASS:
            // Pure single-sided (low-pass character)
            output = low_pass;
            // Scale from Lazzarini eq. 4.8: sqrt(1 - a²)
            scale = vrsqrteq_f32(vsubq_f32(rs->one, a_sq));
            break;

        case RESONANT_MODE_BANDPASS:
            // Mixed response (original resonant synthesis)
            output = vaddq_f32(vmulq_f32(low_pass, vsubq_f32(rs->one, rs->mix)),
                               vmulq_f32(band_pass, rs->mix));
            // Combined scaling
            scale = vaddq_f32(vmulq_f32(vrsqrteq_f32(vsubq_f32(rs->one, a_sq)),
                                         vsubq_f32(rs->one, rs->mix)),
                              vmulq_f32(rs->one, rs->mix));
            break;

        case RESONANT_MODE_HIGHPASS:
            // High-pass derived from low-pass: 1 - low_pass
            output = vsubq_f32(rs->one, low_pass);
            scale = rs->one;
            break;

        case RESONANT_MODE_NOTCH:
            // Notch: 1 - band_pass
            output = vsubq_f32(rs->one, band_pass);
            scale = rs->one;
            break;

        case RESONANT_MODE_PEAK:
            // Peak: band_pass boosted by resonance
            output = vmulq_f32(band_pass, vaddq_f32(rs->one, a));
            scale = vmulq_f32(vrsqrteq_f32(vsubq_f32(rs->one, a_sq)), rs->gain);
            break;

        default:
            output = band_pass;
            scale = rs->one;
    }

    // Apply scaling and voice mask
    output = vmulq_f32(output, scale);
    output = vbslq_f32(vreinterpretq_f32_u32(voice_mask),
                       output, vdupq_n_f32(0.0f));

    return output;
}

/**
 * Get current center frequency (for LFO modulation)
 */
fast_inline float32x4_t resonant_synth_get_center(const resonant_synth_t* rs) {
    return rs->fc;
}

/**
 * Get current resonance value (0-1 scale)
 */
fast_inline float32x4_t resonant_synth_get_resonance(const resonant_synth_t* rs) {
    return rs->resonance;
}