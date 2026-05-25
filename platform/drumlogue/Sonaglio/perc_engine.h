#pragma once

/**
 *  @file perc_engine.h
 *  @brief 3-operator FM percussion / tom / block engine
 *
 *  Parameters:
 *    Param1: Attack / Energy / Brightness
 *    Param2: Body / Decay / Stability
 *
 *  Design intent:
 *  - Attack makes the hit sharper, brighter, and more digital.
 *  - Body makes the sound fuller, lower, more stable, and more tom/block-like.
 *
 *  This engine is the flexible struck-object voice in Sonaglio.
 *  It is not intended to be acoustically realistic; it should produce a clear
 *  percussive hit with enough body and enough character to stand beside Kick,
 *  Snare, and Metal.
 */

#include <arm_neon.h>
#include "sine_neon.h"
#include "fm_voices.h"
#include "engine_mapping.h"

#define PERC_CARRIER_BASE 200.0f
#define PERC_FREQ_MIN      45.0f
#define PERC_FREQ_MAX    1200.0f

typedef struct {
    // Three operators
    float32x4_t phase[3];

    // Base frequency
    float32x4_t carrier_freq_base;

    // UI controls
    float32x4_t attack;          // 0..1
    float32x4_t body;            // 0..1

    // Derived controls, computed in update()
    float32x4_t ratio_center;    // main body ratio
    float32x4_t mod2_base_ratio; // attack / color ratio offset
    float32x4_t pitch_lift;      // short transient pitch lift in octaves
    float32x4_t body_index;      // stable FM body
    float32x4_t strike_index;    // short transient FM complexity
    float32x4_t output_gain;     // body compensation
    float32x4_t drive;           // attack saturation amount
} perc_engine_t;

/**
 * Initialize perc engine
 */
fast_inline void perc_engine_init(perc_engine_t* perc) {
    for (int i = 0; i < 3; i++) {
        perc->phase[i] = vdupq_n_f32(0.0f);
    }

    perc->carrier_freq_base = vdupq_n_f32(PERC_CARRIER_BASE);

    perc->attack = vdupq_n_f32(0.5f);
    perc->body = vdupq_n_f32(0.5f);

    perc->ratio_center = vdupq_n_f32(1.55f);
    perc->mod2_base_ratio = vdupq_n_f32(2.15f);
    perc->pitch_lift = vdupq_n_f32(0.22f);
    perc->body_index = vdupq_n_f32(0.65f);
    perc->strike_index = vdupq_n_f32(1.35f);
    perc->output_gain = vdupq_n_f32(0.78f);
    perc->drive = vdupq_n_f32(0.25f);
}

/**
 * Update perc engine parameters from UI
 */
fast_inline void perc_engine_update(perc_engine_t* perc,
                                    float32x4_t param1,   // Attack / Energy / Brightness
                                    float32x4_t param2) { // Body / Decay / Stability
    const float32x4_t zero = vdupq_n_f32(0.0f);
    const float32x4_t one  = vdupq_n_f32(1.0f);

    perc->attack = vmaxq_f32(zero, vminq_f32(one, param1));
    perc->body   = vmaxq_f32(zero, vminq_f32(one, param2));

    float32x4_t inv_body = vsubq_f32(one, perc->body);

    // Main body ratio:
    // Body should make the instrument more stable, not more chaotic.
    // Range roughly 1.05..2.20, with Attack adding only a small bright shift.
    perc->ratio_center = vaddq_f32(vdupq_n_f32(1.05f),
                                   vaddq_f32(vmulq_n_f32(perc->body, 0.90f),
                                              vmulq_n_f32(perc->attack, 0.25f)));

    // Secondary ratio carries the "digital struck object" color.
    // Low body + high attack = more inharmonic block/wood edge.
    perc->mod2_base_ratio = vaddq_f32(perc->ratio_center,
                                      vaddq_f32(vmulq_n_f32(perc->attack, 0.85f),
                                                 vmulq_n_f32(inv_body, 0.55f)));

    // Short pitch lift at the attack. This decays downward naturally because
    // process() multiplies it by env4.
    perc->pitch_lift = vaddq_f32(vdupq_n_f32(0.03f),
                                 vaddq_f32(vmulq_n_f32(perc->attack, 0.22f),
                                            vmulq_n_f32(inv_body, 0.10f)));

    // Stable body index. Kept moderate so it does not become a melodic FM tone.
    perc->body_index = vaddq_f32(vdupq_n_f32(0.22f),
                                 vmulq_n_f32(perc->body, 0.95f));

    // Short strike index. This is where Attack gets most of its personality.
    // Widened range: 0.30..3.00 (was 0.35..2.05) for more dramatic FM attack.
    perc->strike_index = vaddq_f32(vdupq_n_f32(0.30f),
                                   vaddq_f32(vmulq_n_f32(perc->attack, 2.20f),
                                              vmulq_n_f32(inv_body, 0.50f)));

    // Body makes the hit feel present; Attack adds a little controlled push.
    perc->output_gain = vaddq_f32(vdupq_n_f32(0.55f),
                                  vaddq_f32(vmulq_n_f32(perc->body, 0.35f),
                                             vmulq_n_f32(perc->attack, 0.12f)));

    // Drive is short and attack-weighted.
    perc->drive = vaddq_f32(vdupq_n_f32(0.08f),
                            vmulq_n_f32(perc->attack, 0.62f));
}

/**
 * Set MIDI note (tunable percussion)
 */
fast_inline void perc_engine_set_note(perc_engine_t* perc,
                                      uint32x4_t voice_mask,
                                      float32x4_t midi_notes) {
    float32x4_t base_freq = fm_midi_to_freq(midi_notes, PERC_FREQ_MIN, PERC_FREQ_MAX);
    perc->carrier_freq_base = vbslq_f32(voice_mask,
                                        base_freq,
                                        perc->carrier_freq_base);

    // Reset phases for repeatable front-edge behavior.
    float32x4_t zero = vdupq_n_f32(0.0f);
    for (int i = 0; i < 3; i++) {
        perc->phase[i] = vbslq_f32(voice_mask, zero, perc->phase[i]);
    }
}

/**
 * Process one sample of perc engine
 */
fast_inline float32x4_t perc_engine_process(perc_engine_t* perc,
                                            float32x4_t envelope,
                                            uint32x4_t active_mask,
                                            float32x4_t lfo_pitch_mult,
                                            float32x4_t lfo_index_add) {
    const float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI * INV_SAMPLE_RATE);
    const float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);

    float32x4_t env2 = vmulq_f32(envelope, envelope);
    float32x4_t env4 = vmulq_f32(env2, env2);
    float32x4_t env8 = vmulq_f32(env4, env4);

    // Short transient pitch lift. This makes the hit read as a strike instead
    // of a static FM tone.
    float32x4_t pitch_mult = exp2_neon(vmulq_f32(env8, perc->pitch_lift));

    float32x4_t carrier_freq = vmulq_f32(perc->carrier_freq_base, lfo_pitch_mult);
    carrier_freq = vmulq_f32(carrier_freq, pitch_mult);

    // Modulator frequencies.
    float32x4_t mod1_freq = vmulq_f32(carrier_freq, perc->ratio_center);

    // Mod2 bends very briefly at attack for a block/wood/digital thwack.
    float32x4_t mod2_ratio = vaddq_f32(perc->mod2_base_ratio,
                                       vmulq_n_f32(env2, 0.22f));
    float32x4_t mod2_freq = vmulq_f32(carrier_freq, mod2_ratio);

    // Phase update and wrap.
    perc->phase[0] = vaddq_f32(perc->phase[0], vmulq_f32(carrier_freq, two_pi_over_sr));
    perc->phase[1] = vaddq_f32(perc->phase[1], vmulq_f32(mod1_freq, two_pi_over_sr));
    perc->phase[2] = vaddq_f32(perc->phase[2], vmulq_f32(mod2_freq, two_pi_over_sr));

    for (int i = 0; i < 3; i++) {
        uint32x4_t wrap = vcgeq_f32(perc->phase[i], two_pi);
        perc->phase[i] = vbslq_f32(wrap,
                                   vsubq_f32(perc->phase[i], two_pi),
                                   perc->phase[i]);
    }

    // FM stack:
    // - Op2 provides the stable body.
    // - Op3 provides the short, brighter strike.
    float32x4_t mod1 = neon_sin_fast(perc->phase[1]);
    float32x4_t mod2 = neon_sin_fast(perc->phase[2]);

    float32x4_t body_part = vmulq_f32(mod1,
                                      vmulq_f32(env4, perc->body_index));
    float32x4_t strike_part = vmulq_f32(mod2,
                                        vmulq_f32(env8, perc->strike_index));

    float32x4_t modulation = vaddq_f32(body_part, strike_part);
    modulation = vaddq_f32(modulation, lfo_index_add);

    float32x4_t modulated_phase = vaddq_f32(perc->phase[0],
                                            vmulq_n_f32(modulation, 1.85f));

    float32x4_t body = neon_sin_fast(modulated_phase);
    float32x4_t transient = vmulq_f32(mod2, vmulq_f32(env8, vdupq_n_f32(0.18f)));
    float32x4_t output = vaddq_f32(vmulq_f32(body, envelope), transient);

    // Short attack saturation for more "hit" without making the tail harsh.
    float32x4_t drive_gain = vaddq_f32(vdupq_n_f32(1.0f),
                                       vmulq_f32(perc->drive, env8));
    output = vmulq_f32(output, drive_gain);
    output = fm_cubic_clip(output);

    // Output gain only: envelope is already applied to body/transient above.
    output = vmulq_f32(output, perc->output_gain);

    return vbslq_f32(active_mask, output, vdupq_n_f32(0.0f));
}
