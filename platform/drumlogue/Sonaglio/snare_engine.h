#pragma once

/**
 *  @file snare_engine.h
 *  @brief 2-operator FM snare with per-component independent decay
 *
 *  Parameters:
 *    Param1: Attack / Crack / Wire energy / Transient brightness
 *    Param2: Body / Shell weight / Tonal decay length
 *
 *  Design (ctag-inspired, implemented from first principles):
 *  The defining trait of a snare is that its components decay at DIFFERENT
 *  rates — the tonal shell rings briefly, the FM crack is instantaneous, and
 *  the wire buzz sustains the longest. The previous version approximated this
 *  with power-laws (env^2, env^8) of the already-curved shared ROM envelope,
 *  which compounded unpredictably and killed the body.
 *
 *  This version gives each component its own one-pole exponential decay with
 *  an ABSOLUTE time constant (independent of the shared envelope shape):
 *    - tone_level   : shell tone amplitude + sustained FM index   (35..120 ms)
 *    - noise_level  : wire buzz amplitude — sustains the longest   (50..150 ms)
 *    - index_level  : transient FM crack brightness — very fast    (7..23 ms)
 *
 *  The shared ROM `envelope` is then applied as a linear master VCA, so
 *  ENV_SHAPE still sets the overall length and note-off still cuts the sound,
 *  but the per-component RELATIVE timing (what makes it read as a snare) is
 *  owned by the internal decays. This is a clean product of two well-behaved
 *  envelopes — no power-law compounding.
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

    // Derived FM / mix controls, updated by snare_engine_update()
    float32x4_t mod_ratio;
    float32x4_t pitch_lift;
    float32x4_t tone_index_base;   // sustained shell FM index (follows tone_level)
    float32x4_t crack_index_base;  // transient crack FM index (follows index_level)
    float32x4_t noise_mix;
    float32x4_t tone_gain;
    float32x4_t noise_gain;
    float32x4_t click_gain;
    float32x4_t drive;

    // Per-component independent decay (ctag-style).
    // level decays from 1.0 at its own absolute rate: level -= level * coeff.
    float32x4_t tone_level;
    float32x4_t noise_level;
    float32x4_t index_level;
    float32x4_t tone_decay;        // per-sample one-pole coeff (= 1 / tau_samples)
    float32x4_t noise_decay;
    float32x4_t index_decay;

    // Noise section
    one_pole_t  noise_hpf;
    one_pole_t  noise_lpf;
    one_pole_t  noise_lowp;
    neon_prng_t noise_prng;
} snare_engine_t;

/**
 * One-pole decay coefficient from a 1/e time constant in milliseconds.
 * coeff = 1 / (tau_ms * samples_per_ms). Applied as level -= level * coeff,
 * giving an exponential that reaches ~37% after tau_ms and ~1% after ~4.6*tau_ms.
 */
static inline float32x4_t snare_decay_coeff(float32x4_t tau_ms) {
    float32x4_t samples = vmulq_n_f32(tau_ms, SAMPLE_RATE * 0.001f);
    float32x4_t r = vrecpeq_f32(samples);
    r = vmulq_f32(r, vrecpsq_f32(samples, r));
    r = vmulq_f32(r, vrecpsq_f32(samples, r));
    // Clamp to a stable range: tiny positive floor, 0.5 ceiling.
    return vminq_f32(vdupq_n_f32(0.5f), vmaxq_f32(vdupq_n_f32(1.0e-5f), r));
}

/**
 * Update snare engine parameters from UI
 */
fast_inline void snare_engine_update(snare_engine_t* snare,
                                     float32x4_t param1,   // Attack / Crack / Wire
                                     float32x4_t param2) { // Body / Shell / Decay
    const float32x4_t zero = vdupq_n_f32(0.0f);
    const float32x4_t one  = vdupq_n_f32(1.0f);

    snare->attack = vmaxq_f32(zero, vminq_f32(one, param1));
    snare->body   = vmaxq_f32(zero, vminq_f32(one, param2));

    float32x4_t inv_body = vsubq_f32(one, snare->body);

    // Slightly inharmonic shell ratio. Attack adds dissonance/crack; Body
    // stabilizes it toward shell support. Approx range 1.42 .. 2.42.
    snare->mod_ratio = vaddq_f32(vdupq_n_f32(1.42f),
                                 vaddq_f32(vmulq_n_f32(snare->body, 0.45f),
                                            vmulq_n_f32(snare->attack, 0.55f)));

    // Very short pitch lift for the crack (driven by index_level).
    snare->pitch_lift = vaddq_f32(vdupq_n_f32(0.12f),
                                  vaddq_f32(vmulq_n_f32(snare->attack, 0.65f),
                                             vmulq_n_f32(inv_body, 0.15f)));

    // Sustained shell FM index — follows the tone_level decay. Body deepens it.
    snare->tone_index_base = vaddq_f32(vdupq_n_f32(0.40f),
                                       vmulq_n_f32(snare->body, 1.60f));

    // Transient crack FM index — follows the fast index_level decay.
    snare->crack_index_base = vaddq_f32(vdupq_n_f32(1.00f),
                                        vaddq_f32(vmulq_n_f32(snare->attack, 4.50f),
                                                   vmulq_n_f32(inv_body, 0.35f)));

    // Wire buzz amount. Attack adds it, Body restrains it slightly.
    snare->noise_mix = vaddq_f32(vdupq_n_f32(0.35f),
                                 vsubq_f32(vmulq_n_f32(snare->attack, 0.35f),
                                            vmulq_n_f32(snare->body, 0.15f)));
    snare->noise_mix = vmaxq_f32(vdupq_n_f32(0.10f), vminq_f32(vdupq_n_f32(0.90f), snare->noise_mix));

    // Output balances.
    snare->tone_gain  = vaddq_f32(vdupq_n_f32(0.50f), vmulq_n_f32(snare->body, 0.70f));
    snare->noise_gain = vaddq_f32(vdupq_n_f32(0.20f), vmulq_n_f32(snare->noise_mix, 0.45f));
    snare->click_gain = vaddq_f32(vdupq_n_f32(0.45f), vmulq_n_f32(snare->attack, 1.25f));
    snare->drive      = vaddq_f32(vdupq_n_f32(0.08f), vmulq_n_f32(snare->attack, 0.55f));

    // Per-component decay time constants (1/e), in ms.
    //   tone shell : 35..120 ms — Body lengthens the ring.
    //   wire buzz  : 50..150 ms — sustains longest (Attack + Body extend it).
    //   crack FM   :  7..23  ms — fast front-edge snap (Attack-driven).
    float32x4_t tone_tau  = vaddq_f32(vdupq_n_f32(35.0f), vmulq_n_f32(snare->body, 85.0f));
    float32x4_t noise_tau = vaddq_f32(vdupq_n_f32(50.0f),
                                      vaddq_f32(vmulq_n_f32(snare->attack, 40.0f),
                                                 vmulq_n_f32(snare->body, 60.0f)));
    float32x4_t index_tau = vaddq_f32(vdupq_n_f32(7.0f), vmulq_n_f32(snare->attack, 16.0f));

    snare->tone_decay  = snare_decay_coeff(tone_tau);
    snare->noise_decay = snare_decay_coeff(noise_tau);
    snare->index_decay = snare_decay_coeff(index_tau);
}

/**
 * Initialize snare engine
 */
fast_inline void snare_engine_init(snare_engine_t* snare) {
    snare->carrier_phase = vdupq_n_f32(0.0f);
    snare->modulator_phase = vdupq_n_f32(0.0f);
    snare->carrier_freq_base = vdupq_n_f32(SNARE_CARRIER_BASE);

    snare->attack = vdupq_n_f32(0.5f);
    snare->body = vdupq_n_f32(0.5f);

    // Decayed (silent) at rest; reset to 1.0 on trigger.
    snare->tone_level  = vdupq_n_f32(0.0f);
    snare->noise_level = vdupq_n_f32(0.0f);
    snare->index_level = vdupq_n_f32(0.0f);

    snare->noise_hpf.z1 = vdupq_n_f32(0.0f);
    snare->noise_lpf.z1 = vdupq_n_f32(0.0f);
    snare->noise_lowp.z1 = vdupq_n_f32(0.0f);

    {
        uint64_t s0[2] = { 0xDEADBEEFCAFEBABEULL, 0x0123456789ABCDEFULL };
        uint64_t s1[2] = { 0xFEDCBA9876543210ULL, 0xA5A5A5A5B4B4B4B4ULL };
        snare->noise_prng.state0 = vld1q_u64(s0);
        snare->noise_prng.state1 = vld1q_u64(s1);
    }

    // Derive all FM/mix controls and decay coefficients from default params.
    snare_engine_update(snare, snare->attack, snare->body);
}

/**
 * Set MIDI note for snare. Resets phases and re-arms the per-component decays.
 */
fast_inline void snare_engine_set_note(snare_engine_t* snare,
                                       uint32x4_t voice_mask,
                                       float32x4_t midi_notes) {
    // Callers pass full-bit masks (0xFFFFFFFF per active lane).
    float32x4_t base_freq = fm_midi_to_freq(midi_notes, SNARE_FREQ_MIN, SNARE_FREQ_MAX);
    snare->carrier_freq_base = vbslq_f32(voice_mask,
                                         base_freq,
                                         snare->carrier_freq_base);

    // Reset phases for a repeatable crack.
    float32x4_t zero = vdupq_n_f32(0.0f);
    float32x4_t one  = vdupq_n_f32(1.0f);
    snare->carrier_phase = vbslq_f32(voice_mask, zero, snare->carrier_phase);
    snare->modulator_phase = vbslq_f32(voice_mask, zero, snare->modulator_phase);

    // Re-arm per-component decays to full level for triggered lanes.
    snare->tone_level  = vbslq_f32(voice_mask, one, snare->tone_level);
    snare->noise_level = vbslq_f32(voice_mask, one, snare->noise_level);
    snare->index_level = vbslq_f32(voice_mask, one, snare->index_level);

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
 * Process one sample of snare engine.
 *
 * The shared `envelope` acts only as a linear master VCA (onset attack +
 * ENV_SHAPE length + note-off release). The decay character comes entirely
 * from the per-component internal decays advanced below.
 */
fast_inline float32x4_t snare_engine_process(snare_engine_t* snare,
                                             float32x4_t envelope,
                                             uint32x4_t active_mask,
                                             float32x4_t lfo_pitch_mult,
                                             float32x4_t lfo_index_add,
                                             float32x4_t noise_add) {
    const float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI * INV_SAMPLE_RATE);
    const float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);

    // Advance per-component independent decays (one-pole exponential).
    snare->tone_level  = vsubq_f32(snare->tone_level,
                                   vmulq_f32(snare->tone_level,  snare->tone_decay));
    snare->noise_level = vsubq_f32(snare->noise_level,
                                   vmulq_f32(snare->noise_level, snare->noise_decay));
    snare->index_level = vsubq_f32(snare->index_level,
                                   vmulq_f32(snare->index_level, snare->index_decay));

    // index_level squared → an even faster, clean transient for the click.
    float32x4_t click_env = vmulq_f32(snare->index_level, snare->index_level);

    // Very short pitch lift for a sharper crack (follows index_level).
    float32x4_t pitch_mult = exp2_neon(vmulq_f32(snare->index_level, snare->pitch_lift));

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

    // FM index: sustained shell (tone_level) + transient crack (index_level).
    float32x4_t index = vaddq_f32(vmulq_f32(snare->tone_level, snare->tone_index_base),
                                  vmulq_f32(snare->index_level, snare->crack_index_base));
    index = vaddq_f32(index, lfo_index_add);

    float32x4_t modulator = neon_sin_fast(snare->modulator_phase);
    float32x4_t modulated_phase = vaddq_f32(snare->carrier_phase,
                                            vmulq_f32(modulator, index));
    float32x4_t tone = neon_sin_fast(modulated_phase);

    // Noise blend, with LFO/noise modulation clamped.
    float32x4_t mix = vaddq_f32(snare->noise_mix, noise_add);
    mix = vmaxq_f32(vdupq_n_f32(0.0f), vminq_f32(vdupq_n_f32(1.0f), mix));

    float32x4_t noise = snare_generate_noise(snare);

    // Tone shell: FM tone + low carrier reinforcement, both following tone_level.
    float32x4_t tone_amp = vmulq_f32(snare->tone_level, snare->tone_gain);
    float32x4_t low_shell = vmulq_f32(neon_sin_fast(snare->carrier_phase),
                                      vmulq_f32(snare->tone_level,
                                                vmulq_n_f32(snare->body, 0.30f)));
    float32x4_t tone_out = vaddq_f32(vmulq_f32(tone, tone_amp), low_shell);

    // Wire buzz: the defining snare character — sustains via noise_level.
    float32x4_t wire_gain = vmulq_f32(snare->noise_level,
                                      vmulq_f32(snare->noise_gain,
                                                vaddq_f32(vdupq_n_f32(0.45f),
                                                          vmulq_n_f32(mix, 0.55f))));
    // Sharp front-edge click: click_env (index_level^2) — gone almost immediately.
    float32x4_t click = vmulq_f32(noise, vmulq_f32(click_env, snare->click_gain));
    float32x4_t wire_out = vaddq_f32(vmulq_f32(noise, wire_gain), click);

    // Master VCA: shared ROM envelope (linear) gates onset + length + note-off.
    float32x4_t output = vmulq_f32(vaddq_f32(tone_out, wire_out), envelope);

    // Short transient saturation (driven by the fast click_env), not a tail effect.
    float32x4_t drive_gain = vaddq_f32(vdupq_n_f32(1.0f),
                                       vmulq_f32(snare->drive, click_env));
    output = vmulq_f32(output, drive_gain);
    output = fm_cubic_clip(output);

    return vbslq_f32(active_mask, output, vdupq_n_f32(0.0f));
}
