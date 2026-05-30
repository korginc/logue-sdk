#pragma once

/**
 *  @file snare_engine.h
 *  @brief 2-operator FM snare with transient-focused noise injection
 *
 *  Parameters:
 *    Param1: Attack / Energy / Brightness
 *    Param2: Body / Decay / Stability
 *
 *  Design intent:
 *  - Attack increases crack, wire energy, transient brightness, and front-edge drive.
 *  - Body increases shell weight and tonal support, while keeping the sound controlled.
 *
 *  The snare should be punchy and digital/percussive rather than acoustically realistic.
 */

#include <arm_neon.h>
#include "fm_voices.h"
#include "sine_neon.h"
#include "prng.h"
#include "engine_mapping.h"

#define SNARE_CARRIER_BASE       200.0f
#define SNARE_FREQ_MIN            90.0f
#define SNARE_FREQ_MAX           650.0f
#define SNARE_NOISE_HPF_CUTOFF   900.0f
#define SNARE_NOISE_LPF_CUTOFF  6200.0f
#define SNARE_NOISE_LOWP_CUTOFF  2200.0f

typedef struct {
    // FM operators
    float32x4_t carrier_phase;
    float32x4_t modulator_phase;
    float32x4_t carrier_freq_base;

    // UI controls
    float32x4_t attack;       // 0..1
    float32x4_t body;         // 0..1

    // Derived internal controls, updated by snare_engine_update()
    float32x4_t mod_ratio;
    float32x4_t pitch_lift;
    float32x4_t body_index;
    float32x4_t crack_index;
    float32x4_t noise_mix;
    float32x4_t tone_gain_base;
    float32x4_t noise_gain_base;
    float32x4_t click_gain;
    float32x4_t drive;

    // Noise section
    one_pole_t  noise_hpf;
    one_pole_t  noise_lpf;
    one_pole_t  noise_lowp;
    neon_prng_t noise_prng;
} snare_engine_t;

/**
 * Initialize snare engine
 */
fast_inline void snare_engine_init(snare_engine_t* snare) {
    snare->carrier_phase = vdupq_n_f32(0.0f);
    snare->modulator_phase = vdupq_n_f32(0.0f);
    snare->carrier_freq_base = vdupq_n_f32(SNARE_CARRIER_BASE);

    snare->attack = vdupq_n_f32(0.5f);
    snare->body = vdupq_n_f32(0.5f);

    snare->mod_ratio = vdupq_n_f32(1.70f);
    snare->pitch_lift = vdupq_n_f32(0.20f);
    snare->body_index = vdupq_n_f32(0.75f);
    snare->crack_index = vdupq_n_f32(2.20f);
    snare->noise_mix = vdupq_n_f32(0.45f);
    snare->tone_gain_base = vdupq_n_f32(0.60f);
    snare->noise_gain_base = vdupq_n_f32(0.55f);
    snare->click_gain = vdupq_n_f32(1.20f);
    snare->drive = vdupq_n_f32(0.30f);

    snare->noise_hpf.z1 = vdupq_n_f32(0.0f);
    snare->noise_lpf.z1 = vdupq_n_f32(0.0f);
    snare->noise_lowp.z1 = vdupq_n_f32(0.0f);

    {
        uint64_t s0[2] = { 0xDEADBEEFCAFEBABEULL, 0x0123456789ABCDEFULL };
        uint64_t s1[2] = { 0xFEDCBA9876543210ULL, 0xA5A5A5A5B4B4B4B4ULL };
        snare->noise_prng.state0 = vld1q_u64(s0);
        snare->noise_prng.state1 = vld1q_u64(s1);
    }
}

/**
 * Update snare engine parameters from UI
 */
fast_inline void snare_engine_update(snare_engine_t* snare,
                                     float32x4_t param1,   // Attack / Energy / Brightness
                                     float32x4_t param2) { // Body / Decay / Stability
    const float32x4_t zero = vdupq_n_f32(0.0f);
    const float32x4_t one  = vdupq_n_f32(1.0f);

    snare->attack = vmaxq_f32(zero, vminq_f32(one, param1));
    snare->body   = vmaxq_f32(zero, vminq_f32(one, param2));

    float32x4_t inv_body = vsubq_f32(one, snare->body);

    // Slightly inharmonic shell ratio.
    // Attack makes it more dissonant/cracky; Body stabilizes it toward shell support.
    // Approx range: 1.38 .. 2.25.
    snare->mod_ratio = vaddq_f32(vdupq_n_f32(1.42f),
                                 vaddq_f32(vmulq_n_f32(snare->body, 0.45f),
                                            vmulq_n_f32(snare->attack, 0.55f)));

    // Very short pitch lift for the shell/crack.
    snare->pitch_lift = vaddq_f32(vdupq_n_f32(0.12f),
                                  vaddq_f32(vmulq_n_f32(snare->attack, 0.65f),
                                             vmulq_n_f32(inv_body, 0.15f)));

    // Sustained shell FM. Kept moderate so the snare does not become a pitched tom.
    snare->body_index = vaddq_f32(vdupq_n_f32(0.55f),
                                  vmulq_n_f32(snare->body, 1.85f));

    // Short front-edge crack FM.
    snare->crack_index = vaddq_f32(vdupq_n_f32(1.25f),
                                   vaddq_f32(vmulq_n_f32(snare->attack, 4.20f),
                                              vmulq_n_f32(inv_body, 0.35f)));

    // Attack adds wire/noise. Body slightly restrains noise so body-heavy snares
    // remain more shell-like.
    snare->noise_mix = vaddq_f32(vdupq_n_f32(0.04f),
                                 vsubq_f32(vmulq_n_f32(snare->attack, 0.30f),
                                            vmulq_n_f32(snare->body, 0.20f)));
    snare->noise_mix = vmaxq_f32(zero, vminq_f32(one, snare->noise_mix));

    // Precomputed output balances.
    snare->tone_gain_base = vaddq_f32(vdupq_n_f32(0.85f),
                                      vmulq_n_f32(snare->body, 1.05f));

    snare->noise_gain_base = vaddq_f32(vdupq_n_f32(0.025f),
                                       vmulq_n_f32(snare->noise_mix, 0.34f));

    snare->click_gain = vaddq_f32(vdupq_n_f32(0.85f),
                                  vmulq_n_f32(snare->attack, 2.45f));

    snare->drive = vaddq_f32(vdupq_n_f32(0.08f),
                             vmulq_n_f32(snare->attack, 0.58f));
}

/**
 * Set MIDI note for snare
 */
fast_inline void snare_engine_set_note(snare_engine_t* snare,
                                       uint32x4_t voice_mask,
                                       float32x4_t midi_notes) {
    float32x4_t base_freq = fm_midi_to_freq(midi_notes, SNARE_FREQ_MIN, SNARE_FREQ_MAX);
    snare->carrier_freq_base = vbslq_f32(voice_mask,
                                         base_freq,
                                         snare->carrier_freq_base);

    // Reset phases for a repeatable crack.
    float32x4_t zero = vdupq_n_f32(0.0f);
    snare->carrier_phase = vbslq_f32(voice_mask, zero, snare->carrier_phase);
    snare->modulator_phase = vbslq_f32(voice_mask, zero, snare->modulator_phase);

    // Do not reset noise filter state here. Keeping it continuous avoids
    // hard clicks and makes repeated hits less mechanically identical.
}

/**
 * Generate bandpassed noise sample for all 4 lanes
 */
fast_inline float32x4_t snare_generate_noise(snare_engine_t* snare) {
    float32x4_t white = white_noise(&snare->noise_prng);
    float32x4_t low = one_pole_lpf(&snare->noise_lowp, white, SNARE_NOISE_LOWP_CUTOFF);
    float32x4_t hp_lp = one_pole_lpf(&snare->noise_hpf, white, SNARE_NOISE_HPF_CUTOFF);
    float32x4_t high = vsubq_f32(white, hp_lp);
    float32x4_t band = one_pole_lpf(&snare->noise_lpf, high, SNARE_NOISE_LPF_CUTOFF);
    return vaddq_f32(vmulq_n_f32(low, 0.18f),
                     vaddq_f32(vmulq_n_f32(band, 0.62f),
                               vmulq_n_f32(high, 0.20f)));
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
    const float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI * INV_SAMPLE_RATE);
    const float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);

    float32x4_t env2 = vmulq_f32(envelope, envelope);
    float32x4_t env4 = vmulq_f32(env2, env2);
    float32x4_t env8 = vmulq_f32(env4, env4);
    // Explicit domains per synthesis block.
    float32x4_t amp_env = vmulq_f32(envelope, envelope); // Snappier shell decay
    float32x4_t pitch_env = env8;        // keep pitch transient short
    float32x4_t index_env = env8;        // FM brightness dies quickly
    float32x4_t noise_env = env4;        // Tighten the noise burst

    // Very short pitch lift for a sharper crack.
    float32x4_t pitch_mult = exp2_neon(vmulq_f32(pitch_env, snare->pitch_lift));

    float32x4_t carrier_freq = vmulq_f32(snare->carrier_freq_base, lfo_pitch_mult);
    carrier_freq = vmulq_f32(carrier_freq, pitch_mult);

    float32x4_t mod_freq = vmulq_f32(carrier_freq, snare->mod_ratio);

    snare->carrier_phase = vaddq_f32(snare->carrier_phase,
                                     vmulq_f32(carrier_freq, two_pi_over_sr));
    snare->modulator_phase = vaddq_f32(snare->modulator_phase,
                                       vmulq_f32(mod_freq, two_pi_over_sr));

    uint32x4_t c_wrap = vcgeq_f32(snare->carrier_phase, two_pi);
    uint32x4_t m_wrap = vcgeq_f32(snare->modulator_phase, two_pi);
    snare->carrier_phase = vbslq_f32(c_wrap,
                                     vsubq_f32(snare->carrier_phase, two_pi),
                                     snare->carrier_phase);
    snare->modulator_phase = vbslq_f32(m_wrap,
                                       vsubq_f32(snare->modulator_phase, two_pi),
                                       snare->modulator_phase);

    // FM index:
    // - body_index gives a short shell/body tone
    // - crack_index is very transient
    float32x4_t index = vaddq_f32(vmulq_f32(env2, snare->body_index),
                                  vmulq_f32(index_env, snare->crack_index));
    index = vaddq_f32(index, lfo_index_add);

    float32x4_t modulator = neon_sin_fast(snare->modulator_phase);
    float32x4_t modulated_phase = vaddq_f32(snare->carrier_phase,
                                            vmulq_f32(modulator, index));

    float32x4_t tone = neon_sin_fast(modulated_phase);

    // Noise blend, with LFO/noise modulation clamped.
    float32x4_t mix = vaddq_f32(snare->noise_mix, noise_add);
    mix = vmaxq_f32(vdupq_n_f32(0.0f), vminq_f32(vdupq_n_f32(1.0f), mix));

    float32x4_t noise = snare_generate_noise(snare);

    // Tone carries the shell whack; keep it present longer than the noise burst.
    float32x4_t tone_gain = vmulq_f32(amp_env, snare->tone_gain_base);

    // Noise can last with the envelope, but its level remains attack-weighted.
    float32x4_t noise_gain = vmulq_f32(noise_env,
                                       vmulq_f32(snare->noise_gain_base,
                                                 vaddq_f32(vdupq_n_f32(0.42f),
                                                           vmulq_n_f32(mix, 0.58f))));

    // Very short front click from the same bandpassed noise.
    float32x4_t click = vmulq_f32(noise,
                                  vmulq_f32(env8, snare->click_gain));

    float32x4_t low_shell = vmulq_f32(neon_sin_fast(snare->carrier_phase),
                                      vmulq_f32(env2, vmulq_n_f32(snare->body, 0.38f)));
    float32x4_t body = vaddq_f32(vmulq_f32(tone, tone_gain), low_shell);
    float32x4_t transient = vaddq_f32(vmulq_f32(noise, vmulq_n_f32(noise_gain, 0.38f)), click);
    float32x4_t noise_tail = vmulq_f32(noise, vmulq_f32(noise_gain, amp_env));
    float32x4_t output = vaddq_f32(body, vaddq_f32(transient, noise_tail));

    // Short transient saturation, not a long tail distortion.
    float32x4_t drive_gain = vaddq_f32(vdupq_n_f32(1.0f),
                                       vmulq_f32(snare->drive, env8));
    output = vmulq_f32(output, drive_gain);
    output = fm_cubic_clip(output);

    return vbslq_f32(active_mask, output, vdupq_n_f32(0.0f));
}
