#pragma once

/**
 *  @file hat_engine.h
 *  @brief Square-wave hi-hat engine (TRX-style)
 *
 *  Parameters:
 *    Param1: Attack / Brightness / Noise level at transient
 *    Param2: Body / Tone mix (low = mostly noise, high = more pitched squares)
 *
 *  Design:
 *  Six detuned square-wave oscillators at fixed metallic ratios, mixed with
 *  bandpassed noise. No sine computation, no exp2 — cheapest possible synthesis.
 *
 *  Oscillator frequencies (Hz): 306, 512, 551, 743, 826, 900
 *  These are the same intervals used in TR-808/909-family metallic oscillators.
 *  MIDI note shifts all six frequencies together as a group.
 */

#include <arm_neon.h>
#include "fm_voices.h"
#include "prng.h"
#include "engine_mapping.h"

#define HAT_OSC_COUNT (6)

// Base frequencies in Hz
static const float HAT_BASE_FREQ[HAT_OSC_COUNT] = {
    306.0f, 512.0f, 551.0f, 743.0f, 826.0f, 900.0f
};

// Frequency ratio for each oscillator (relative to HAT_BASE_FREQ[0])
// Precomputed: freq[i] / freq[0]
static const float HAT_RATIO[HAT_OSC_COUNT] = {
    1.0000f, 1.6732f, 1.8007f, 2.4281f, 2.6993f, 2.9412f
};

#define HAT_FREQ_MIN   200.0f
#define HAT_FREQ_MAX  5000.0f

typedef struct {
    // Six phase accumulators [0..1)
    float32x4_t phase[HAT_OSC_COUNT];

    // Base frequency for oscillator 0 (others follow via HAT_RATIO)
    float32x4_t carrier_freq_base;  // Hz

    // UI controls
    float32x4_t attack;   // 0..1: transient brightness
    float32x4_t body;     // 0..1: tone/noise ratio

    // Derived, updated in hat_engine_update()
    float32x4_t tone_gain;    // contribution of square osc sum
    float32x4_t noise_gain;   // contribution of noise
    float32x4_t click_gain;   // very short noise burst at attack

    // Noise section
    neon_prng_t noise_prng;
    one_pole_t  noise_hpf;
    one_pole_t  noise_lpf;
} hat_engine_t;

/**
 * Initialize hat engine
 */
fast_inline void hat_engine_init(hat_engine_t* hat) {
    for (int i = 0; i < HAT_OSC_COUNT; i++) {
        hat->phase[i] = vdupq_n_f32(0.0f);
    }

    hat->carrier_freq_base = vdupq_n_f32(HAT_BASE_FREQ[0]);

    hat->attack = vdupq_n_f32(0.5f);
    hat->body = vdupq_n_f32(0.5f);

    hat->tone_gain  = vdupq_n_f32(0.30f);
    hat->noise_gain = vdupq_n_f32(0.60f);
    hat->click_gain = vdupq_n_f32(0.80f);

    {
        uint64_t s0[2] = { 0xBEEFCAFEDEAD1234ULL, 0x5678ABCDEF012345ULL };
        uint64_t s1[2] = { 0x9876543210FEDCBAULL, 0xC3D4E5F607182930ULL };
        hat->noise_prng.state0 = vld1q_u64(s0);
        hat->noise_prng.state1 = vld1q_u64(s1);
    }

    hat->noise_hpf.z1 = vdupq_n_f32(0.0f);
    hat->noise_lpf.z1 = vdupq_n_f32(0.0f);
}

/**
 * Update hat engine parameters from UI
 */
fast_inline void hat_engine_update(hat_engine_t* hat,
                                   float32x4_t param1,   // Attack / Brightness
                                   float32x4_t param2) { // Body / Tone mix
    const float32x4_t zero = vdupq_n_f32(0.0f);
    const float32x4_t one  = vdupq_n_f32(1.0f);

    hat->attack = vmaxq_f32(zero, vminq_f32(one, param1));
    hat->body   = vmaxq_f32(zero, vminq_f32(one, param2));

    // More body = more oscillator tone, less noise
    hat->tone_gain  = vaddq_f32(vdupq_n_f32(0.12f), vmulq_n_f32(hat->body, 0.45f));
    hat->noise_gain = vsubq_f32(vdupq_n_f32(0.75f), vmulq_n_f32(hat->body, 0.35f));

    // Attack adds a short noise click for the stick transient
    hat->click_gain = vaddq_f32(vdupq_n_f32(0.30f), vmulq_n_f32(hat->attack, 1.10f));
}

/**
 * Set MIDI note (shifts all oscillator frequencies together)
 */
fast_inline void hat_engine_set_note(hat_engine_t* hat,
                                     uint32x4_t voice_mask,
                                     float32x4_t midi_notes) {
    float32x4_t base_freq = fm_midi_to_freq(midi_notes, HAT_FREQ_MIN, HAT_FREQ_MAX);
    hat->carrier_freq_base = vbslq_f32(voice_mask, base_freq, hat->carrier_freq_base);

    // Phase reset for consistent attack
    float32x4_t zero = vdupq_n_f32(0.0f);
    for (int i = 0; i < HAT_OSC_COUNT; i++) {
        hat->phase[i] = vbslq_f32(voice_mask, zero, hat->phase[i]);
    }
}

/**
 * Process one sample of hat engine.
 *
 * No transcendental functions — square waves only + noise.
 */
fast_inline float32x4_t hat_engine_process(hat_engine_t* hat,
                                           float32x4_t envelope,
                                           uint32x4_t active_mask,
                                           float32x4_t lfo_pitch_mult,
                                           float32x4_t lfo_noise_add) {
    const float32x4_t inv_sr = vdupq_n_f32(INV_SAMPLE_RATE);
    const float32x4_t half   = vdupq_n_f32(0.5f);
    const float32x4_t one    = vdupq_n_f32(1.0f);

    float32x4_t env2 = vmulq_f32(envelope, envelope);
    float32x4_t env8 = vmulq_f32(vmulq_f32(env2, env2), vmulq_f32(env2, env2));

    float32x4_t base = vmulq_f32(hat->carrier_freq_base, lfo_pitch_mult);

    // Advance all six oscillator phases and generate square waves.
    float32x4_t tone = vdupq_n_f32(0.0f);
    for (int i = 0; i < HAT_OSC_COUNT; i++) {
        float32x4_t freq = vmulq_n_f32(base, HAT_RATIO[i]);
        hat->phase[i] = vaddq_f32(hat->phase[i], vmulq_f32(freq, inv_sr));

        // Wrap phase [0..1)
        uint32x4_t over = vcgeq_f32(hat->phase[i], one);
        hat->phase[i] = vbslq_f32(over,
                                   vsubq_f32(hat->phase[i], one),
                                   hat->phase[i]);

        // Square wave: +1 if phase < 0.5, else -1
        uint32x4_t hi = vcltq_f32(hat->phase[i], half);
        float32x4_t sq = vbslq_f32(hi, one, vdupq_n_f32(-1.0f));
        tone = vaddq_f32(tone, sq);
    }

    // Normalize oscillator sum (~6 oscillators, keep headroom)
    tone = vmulq_n_f32(tone, 1.0f / HAT_OSC_COUNT);

    // Noise: white noise → HPF → LPF band
    float32x4_t white = white_noise(&hat->noise_prng);
    float32x4_t hp = one_pole_hpf(&hat->noise_hpf, white, 3000.0f);
    float32x4_t noise = one_pole_lpf(&hat->noise_lpf, hp, 10000.0f);

    // Mix: tone decays with env2 (medium tail), noise with env2, click on env8
    float32x4_t tone_out  = vmulq_f32(tone,  vmulq_f32(env2, hat->tone_gain));
    float32x4_t noise_out = vmulq_f32(noise, vmulq_f32(env2, hat->noise_gain));
    float32x4_t click_out = vmulq_f32(noise, vmulq_f32(env8, hat->click_gain));

    // Attack also lifts the noise floor contribution from lfo_noise_add
    float32x4_t noise_mod = vmulq_f32(noise, vmulq_f32(env8, lfo_noise_add));

    float32x4_t output = vaddq_f32(tone_out,
                                   vaddq_f32(noise_out,
                                             vaddq_f32(click_out, noise_mod)));

    output = fm_cubic_clip(output);
    output = vmulq_f32(output, envelope);

    return vbslq_f32(active_mask, output, vdupq_n_f32(0.0f));
}
