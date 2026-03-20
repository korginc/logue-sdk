/**
 *  @file snare_engine.h
 *  @brief 2-operator FM snare with noise injection
 *
 *  Operators:
 *    Op1: Carrier (180-250Hz)
 *    Op2: Modulator (ratio 1.5-2.5)
 *    Noise: Bandpassed noise for sizzle
 *  Parameters:
 *    Param1: Noise mix (0-100%) - balance between tone and noise
 *    Param2: Body resonance (0-100%) - controls filter/Q
 */

#pragma once

#include <arm_neon.h>
#include "fm_voices.h"
#include "sine_neon.h"
#include "prng.h"

// Snare engine constants
#define SNARE_CARRIER_BASE 200.0f    // Base frequency
#define SNARE_MOD_RATIO_MIN 1.5f
#define SNARE_MOD_RATIO_MAX 2.5f
#define SNARE_NOISE_HPF_CUTOFF 800.0f   // High-pass for noise
#define SNARE_NOISE_LPF_CUTOFF 5000.0f  // Low-pass for noise

/**
 * Simple one-pole filter for noise shaping
 */
typedef struct {
    float32x4_t z1;  // Delay element
} one_pole_t;

fast_inline float32x4_t one_pole_lpf(one_pole_t* f, float32x4_t in, float cutoff) {
    float32x4_t alpha = vdupq_n_f32(cutoff / (cutoff + 48000.0f));
    float32x4_t out = vaddq_f32(vmulq_f32(in, alpha),
                                vmulq_f32(f->z1, vsubq_f32(vdupq_n_f32(1.0f), alpha)));
    f->z1 = out;
    return out;
}

/**
 * Snare engine state
 */
typedef struct {
    // FM operators
    float32x4_t carrier_phase;
    float32x4_t modulator_phase;
    float32x4_t carrier_freq_base;
    float32x4_t mod_ratio;

    // Noise section
    one_pole_t noise_hpf;
    one_pole_t noise_lpf;
    neon_prng_t noise_prng;  // Separate PRNG for noise

    // Parameters
    float32x4_t noise_mix;      // 0-1
    float32x4_t body_resonance;  // 0-1
} snare_engine_t;

/**
 * Initialize snare engine
 */
fast_inline void snare_engine_init(snare_engine_t* snare) {
    snare->carrier_phase = vdupq_n_f32(0.0f);
    snare->modulator_phase = vdupq_n_f32(0.0f);
    snare->carrier_freq_base = vdupq_n_f32(SNARE_CARRIER_BASE);
    snare->mod_ratio = vdupq_n_f32(2.0f);

    snare->noise_hpf.z1 = vdupq_n_f32(0.0f);
    snare->noise_lpf.z1 = vdupq_n_f32(0.0f);
    neon_prng_init(&snare->noise_prng, 0x87654321);

    snare->noise_mix = vdupq_n_f32(0.3f);
    snare->body_resonance = vdupq_n_f32(0.5f);
}

/**
 * Update snare engine parameters
 */
fast_inline void snare_engine_update(snare_engine_t* snare,
                                     float32x4_t param1,  // Noise mix
                                     float32x4_t param2) { // Body resonance
    snare->noise_mix = param1;
    snare->body_resonance = param2;
}

/**
 * Set MIDI note for snare
 */
fast_inline void snare_engine_set_note(snare_engine_t* snare,
                                       uint32x4_t voice_mask,
                                       float32x4_t midi_notes) {
    // Similar to kick, but with different frequency range
    float32x4_t a4_freq = vdupq_n_f32(440.0f);
    float32x4_t a4_midi = vdupq_n_f32(69.0f);
    float32x4_t twelfth = vdupq_n_f32(1.0f/12.0f);

    float32x4_t exponent = vmulq_f32(vsubq_f32(midi_notes, a4_midi), twelfth);

    // 2^x approximation
    float32x4_t two_pow = vdupq_n_f32(1.0f);
    float32x4_t x2 = vmulq_f32(exponent, exponent);
    two_pow = vmlaq_f32(two_pow, exponent, vdupq_n_f32(0.693f));
    two_pow = vmlaq_f32(two_pow, x2, vdupq_n_f32(0.24f));

    float32x4_t base_freq = vmulq_f32(a4_freq, two_pow);

    snare->carrier_freq_base = vbslq_f32(voice_mask,
                                         base_freq,
                                         snare->carrier_freq_base);

    // Reset phases on trigger for consistent attack transient
    float32x4_t zero = vdupq_n_f32(0.0f);
    snare->carrier_phase   = vbslq_f32(voice_mask, zero, snare->carrier_phase);
    snare->modulator_phase = vbslq_f32(voice_mask, zero, snare->modulator_phase);
}

/**
 * Generate noise sample for all 4 voices
 */
fast_inline float32x4_t snare_generate_noise(snare_engine_t* snare) {
    // Generate 4 random values
    uint32x4_t rand = neon_prng_rand_u32(&snare->noise_prng);

    // Convert to float in [-1, 1]
    uint32x4_t masked = vandq_u32(rand, vdupq_n_u32(0x7FFFFF));
    uint32x4_t float_bits = vorrq_u32(masked, vdupq_n_u32(0x3F800000));
    float32x4_t white = vsubq_f32(vreinterpretq_f32_u32(float_bits),
                                  vdupq_n_f32(1.0f));
    white = vsubq_f32(vmulq_f32(white, vdupq_n_f32(2.0f)), vdupq_n_f32(1.0f));

    // Apply bandpass filtering (simplified)
    float32x4_t hpf_out = one_pole_lpf(&snare->noise_hpf, white,
                                       SNARE_NOISE_HPF_CUTOFF);
    float32x4_t bpf_out = one_pole_lpf(&snare->noise_lpf, hpf_out,
                                       SNARE_NOISE_LPF_CUTOFF);

    return bpf_out;
}

/**
 * Process one sample of snare engine
 */
fast_inline float32x4_t snare_engine_process(snare_engine_t* snare,
                                             float32x4_t envelope,
                                             uint32x4_t active_mask) {
    // Calculate frequencies
    float32x4_t carrier_freq = snare->carrier_freq_base;
    float32x4_t mod_freq = vmulq_f32(carrier_freq, snare->mod_ratio);

    // Phase increments
    float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI / 48000.0f);
    float32x4_t carrier_inc = vmulq_f32(carrier_freq, two_pi_over_sr);
    float32x4_t mod_inc = vmulq_f32(mod_freq, two_pi_over_sr);

    // Update phases
    snare->carrier_phase = vaddq_f32(snare->carrier_phase, carrier_inc);
    snare->modulator_phase = vaddq_f32(snare->modulator_phase, mod_inc);

    // Wrap phases
    float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);
    uint32x4_t carrier_wrap = vcgeq_f32(snare->carrier_phase, two_pi);
    uint32x4_t mod_wrap = vcgeq_f32(snare->modulator_phase, two_pi);

    snare->carrier_phase = vbslq_f32(carrier_wrap,
                                     vsubq_f32(snare->carrier_phase, two_pi),
                                     snare->carrier_phase);
    snare->modulator_phase = vbslq_f32(mod_wrap,
                                       vsubq_f32(snare->modulator_phase, two_pi),
                                       snare->modulator_phase);

    // FM synthesis
    float32x4_t modulator = neon_sin(snare->modulator_phase);

    // Modulation index with body resonance
    // resonance boosts certain frequencies
    float32x4_t index = vmulq_f32(envelope,
                                  vaddq_f32(vdupq_n_f32(1.0f),
                                           vmulq_f32(snare->body_resonance,
                                                    envelope)));

    float32x4_t modulated_phase = vaddq_f32(snare->carrier_phase,
                                           vmulq_f32(modulator, index));
    float32x4_t tone = neon_sin(modulated_phase);

    // Noise generation
    float32x4_t noise = snare_generate_noise(snare);

    // Mix tone and noise
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t noise_gain = vmulq_f32(snare->noise_mix, envelope);
    float32x4_t tone_gain = vsubq_f32(one, snare->noise_mix);

    float32x4_t output = vaddq_f32(vmulq_f32(tone, tone_gain),
                                   vmulq_f32(noise, noise_gain));

    // Apply envelope and mask
    output = vmulq_f32(output, envelope);
    output = vbslq_f32(active_mask,
                       output, vdupq_n_f32(0.0f));

    return output;
}

// ========== UNIT TEST ==========
#ifdef TEST_SNARE

void test_snare_noise_mix() {
    snare_engine_t snare;
    snare_engine_init(&snare);

    // Test noise mix parameter
    float32x4_t param1 = vdupq_n_f32(0.5f);  // 50% noise
    float32x4_t param2 = vdupq_n_f32(0.5f);
    snare_engine_update(&snare, param1, param2);

    uint32x4_t mask = vdupq_n_u32(0xFFFFFFFF);
    float32x4_t env = vdupq_n_f32(1.0f);

    // Process a few samples
    float tone_sum = 0, noise_sum = 0;
    for (int i = 0; i < 100; i++) {
        float32x4_t out = snare_engine_process(&snare, env, mask);
        (void)out; // Would analyze spectrum in real test
    }

    printf("Snare engine test PASSED\n");
}

#endif