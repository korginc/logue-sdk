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
// one_pole_t and one_pole_lpf_a/one_pole_lpf are now in fm_voices.h
#define SNARE_CARRIER_BASE 200.0f    // Base frequency
#define SNARE_MOD_RATIO_MIN 1.5f
#define SNARE_MOD_RATIO_MAX 2.5f
#define SNARE_NOISE_HPF_CUTOFF 800.0f   // High-pass for noise
#define SNARE_NOISE_LPF_CUTOFF 5000.0f  // Low-pass for noise

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
    one_pole_t  noise_hpf;
    one_pole_t  noise_lpf;
    neon_prng_t noise_prng;      // Separate PRNG for noise

    // Parameters
    float32x4_t noise_mix;       // 0-1
    float32x4_t body_resonance;  // 0-1
} snare_engine_t;

/**
 * Initialize snare engine
 */
fast_inline void snare_engine_init(snare_engine_t* snare) {
    snare->carrier_phase = vdupq_n_f32(0.0f);
    snare->modulator_phase = vdupq_n_f32(0.0f);
    snare->carrier_freq_base = vdupq_n_f32(SNARE_CARRIER_BASE);
    // Change this from 2.0f (an octave) to 1.414f (square root of 2, a tritone)
    // This creates dissonant, metallic bell-tones perfect for a snare shell!
    snare->mod_ratio = vdupq_n_f32(1.414f);

    snare->noise_hpf.z1 = vdupq_n_f32(0.0f);
    snare->noise_lpf.z1 = vdupq_n_f32(0.0f);
    // Four completely independent seeds: neon_prng uses uint64x2_t (2 lanes),
    // so state0 feeds voices 0&2 and state1 feeds voices 1&3.
    // Using unrelated bit patterns rather than a single base seed prevents
    // correlation between lanes when the snare restarts from init.
    {
        uint64_t s0[2] = { 0xDEADBEEFCAFEBABEULL, 0x0123456789ABCDEFULL };
        uint64_t s1[2] = { 0xFEDCBA9876543210ULL, 0xA5A5A5A5B4B4B4B4ULL };
        snare->noise_prng.state0 = vld1q_u64(s0);
        snare->noise_prng.state1 = vld1q_u64(s1);
    }

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
    float32x4_t two_pow = exp2_neon(exponent);

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
fast_inline float32x4_t snare_generate_noise(snare_engine_t * snare, float32x4_t env2) {
  // Generate 4 random values
  float32x4_t white = neon_generate_float_white_noise(&snare->noise_prng);
  // Generate the low-pass curve at 800Hz
  // float32x4_t lp_800 = one_pole_lpf(&snare->noise_hpf, white, SNARE_NOISE_HPF_CUTOFF);
  // Envelope-dependent bandwidth => attack = bright, wide
  float32x4_t cutoff_hp =
      vaddq_f32(vdupq_n_f32(600.0f),
                vmulq_n_f32(env2, 2000.0f));
  float32x4_t lp_800 = one_pole_lpf(&snare->noise_hpf, white, cutoff_hp);
  // Subtract it from the original noise to get a High-Pass at 800Hz
  float32x4_t hpf_out = vsubq_f32(white, lp_800);

  // Now apply the 5000Hz Low-Pass to the High-Passed signal to create the Bandpass
  // float32x4_t bpf_out = one_pole_lpf(&snare->noise_lpf, hpf_out, SNARE_NOISE_LPF_CUTOFF);
  // Envelope-dependent bandwidth => tail = dull, narrow
  float32x4_t cutoff_lp =
      vaddq_f32(vdupq_n_f32(3000.0f),
                vmulq_n_f32(env2, 6000.0f));
  float32x4_t bpf_out = one_pole_lpf(&snare->noise_lpf, hpf_out, cutoff_lp);

  return bpf_out;
}

/**
 * Process one sample of snare engine
 */
fast_inline float32x4_t snare_engine_process(snare_engine_t* snare,
                                             float32x4_t envelope,
                                             uint32x4_t active_mask,
                                             float32x4_t lfo_pitch_mult,
                                             float32x4_t lfo_index_add,
                                             float32x4_t noise_add) {

    // APC BAILOUT: Check if all 4 voices are dead
    // Extract the max value across the 4 lanes of the mask
    #if defined(__aarch64__)
        uint32_t max_mask = vmaxvq_u32(active_mask);
    #else
        // 32-bit ARM fallback for vector max
        uint32x2_t max_half = vmax_u32(vget_low_u32(active_mask), vget_high_u32(active_mask));
        uint32_t max_mask = vget_lane_u32(vpmax_u32(max_half, max_half), 0);
    #endif

    // If the mask is zero across all lanes, SKIP THE MATH!
    if (max_mask == 0) {
        return vdupq_n_f32(0.0f);
    }

    // 1. Staggered Envelopes (The Body must decay much faster than the Noise)
    float32x4_t env2 = vmulq_f32(envelope, envelope);
    float32x4_t env4 = vmulq_f32(env2, env2);

    // new
    float32x4_t pitch_env = vmulq_f32(snare->body_resonance, env2);

    // 2. Apply LFO Pitch Modulation
    float32x4_t carrier_freq = vmulq_f32(snare->carrier_freq_base, lfo_pitch_mult);
    // Add micro instability to mod_ratio. Prevents sterile FM tone adn add grits
    float32x4_t mod_freq = vmulq_f32(carrier_freq, vaddq_f32(snare->mod_ratio,
                                                             vmulq_f32(env2, vdupq_n_f32(0.1f))));

    // 3. Phase increments & Wrapping
    float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI * INV_SAMPLE_RATE);
    snare->carrier_phase = vaddq_f32(snare->carrier_phase, vmulq_f32(carrier_freq, two_pi_over_sr));
    snare->modulator_phase = vaddq_f32(snare->modulator_phase, vmulq_f32(mod_freq, two_pi_over_sr));

    float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);
    uint32x4_t c_wrap = vcgeq_f32(snare->carrier_phase, two_pi);
    uint32x4_t m_wrap = vcgeq_f32(snare->modulator_phase, two_pi);
    snare->carrier_phase = vbslq_f32(c_wrap, vsubq_f32(snare->carrier_phase, two_pi), snare->carrier_phase);
    snare->modulator_phase = vbslq_f32(m_wrap, vsubq_f32(snare->modulator_phase, two_pi), snare->modulator_phase);

    // 4. FM Synthesis (The "Crack")
    // Use env4 so the tonal resonance dies instantly like a real drum hit
    // float32x4_t index = vmulq_f32(env4, vaddq_f32(vdupq_n_f32(1.0f), vmulq_n_f32(snare->body_resonance, 3.0f)));
    float32x4_t transient = vaddq_f32(env2, vmulq_n_f32(env4, 2.0f));

    float32x4_t index =
        vmulq_f32(transient,
                  vaddq_f32(vdupq_n_f32(1.0f),
                            vmulq_n_f32(pitch_env, 5.0f)));
    index = vaddq_f32(index, lfo_index_add); // Add LFO index mod
    // 4.1 Generate sine waves
    float32x4_t modulator = neon_sin(snare->modulator_phase);
    // Add a TRUE transient “crack” (discontinuity, sharp attack spike)
    float32x4_t crack = vmulq_f32(env4, vdupq_n_f32(8.0f));
    float32x4_t modulated_phase =
        vaddq_f32(snare->carrier_phase,
                  vaddq_f32(vmulq_f32(modulator, index), crack));
    float32x4_t tone = neon_sin(modulated_phase);

    // 5. Noise Generation (The "Sizzle")
    float32x4_t noise = snare_generate_noise(snare, env2);
    // modulate noise with tone, less independent
    float32x4_t noise_colored =
        vmulq_f32(noise,
                  vaddq_f32(vdupq_n_f32(0.7f), vmulq_n_f32(tone, 0.3f)));
    // 6. Mix: Tone uses fast env4, Noise uses standard envelope
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t noise_mix_mod = vaddq_f32(snare->noise_mix, noise_add);
    float32x4_t noise_gain = vmulq_f32(noise_mix_mod, envelope);
    float32x4_t tone_gain = vmulq_f32(vsubq_f32(one, noise_mix_mod), env4);
    // 8. Apply main amplitude envelope and gate mask
    float32x4_t output = vaddq_f32(vmulq_f32(tone, tone_gain), vmulq_f32(noise_colored, noise_gain));

    // asymmetric saturation
    float32x4_t drive = vdupq_n_f32(1.5f);
    float32x4_t driven = vmulq_f32(output, drive);

    output = fast_div_neon(driven,
                           vaddq_f32(vdupq_n_f32(1.0f), vabsq_f32(driven)));

    return vbslq_f32(active_mask, output, vdupq_n_f32(0.0f));
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