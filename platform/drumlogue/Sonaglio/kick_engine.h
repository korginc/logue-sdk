#pragma once

/**
 *  @file kick_engine.h
 *  @brief 2-operator FM kick drum with attack/body controls
 *
 *  Parameters:
 *    Param1: Attack / Energy / Brightness
 *    Param2: Body / Decay / Stability
 *
 *  Design intent:
 *  - Attack makes the front edge harder, brighter, and more driven.
 *  - Body makes the pitch movement more stable and the low end fuller.
 *
 *  Notes:
 *  - This version keeps the 2-op FM structure.
 *  - Expensive/derived macro values are moved into kick_engine_update().
 *  - The process path adds cheap transient drive and body stabilization.
 */

#include <arm_neon.h>
#include "fm_voices.h"
#include "sine_neon.h"
#include "envelope_rom.h"
#include "engine_mapping.h"

// Kick engine constants
#define KICK_CARRIER_BASE   60.0f
#define KICK_FREQ_MIN       20.0f
#define KICK_FREQ_MAX       420.0f

typedef struct {
    float32x4_t carrier_phase;
    float32x4_t modulator_phase;
    float32x4_t carrier_freq_base;

    // UI controls
    float32x4_t attack;       // 0..1
    float32x4_t body;         // 0..1

    // Derived controls, updated outside the audio hot path
    float32x4_t mod_ratio;    // higher attack / lower body = more click complexity
    float32x4_t sweep_depth;  // transient pitch drop depth in octaves
    float32x4_t body_index;   // sustained FM body
    float32x4_t click_index;  // very short FM attack
    float32x4_t output_gain;  // body compensation
    float32x4_t drive;        // transient saturation amount
} kick_engine_t;

/**
 * Initialize kick engine
 */
fast_inline void kick_engine_init(kick_engine_t* kick) {
    kick->carrier_phase = vdupq_n_f32(0.0f);
    kick->modulator_phase = vdupq_n_f32(0.0f);
    kick->carrier_freq_base = vdupq_n_f32(KICK_CARRIER_BASE);

    kick->attack = vdupq_n_f32(0.5f);
    kick->body = vdupq_n_f32(0.5f);

    kick->mod_ratio = vdupq_n_f32(1.65f);
    kick->sweep_depth = vdupq_n_f32(1.65f);
    kick->body_index = vdupq_n_f32(0.70f);
    kick->click_index = vdupq_n_f32(1.50f);
    kick->output_gain = vdupq_n_f32(0.78f);
    kick->drive = vdupq_n_f32(0.35f);
}

/**
 * Update kick engine parameters from UI.
 *
 * Important sound-design change:
 * - Body now makes the kick more stable.
 * - Attack now contributes more to high transient complexity.
 */
fast_inline void kick_engine_update(kick_engine_t* kick,
                                    float32x4_t param1,   // Attack / Energy / Brightness
                                    float32x4_t param2) { // Body / Decay / Stability
    const float32x4_t one = vdupq_n_f32(1.0f);

    kick->attack = vmaxq_f32(vdupq_n_f32(0.0f), vminq_f32(one, param1));
    kick->body   = vmaxq_f32(vdupq_n_f32(0.0f), vminq_f32(one, param2));

    float32x4_t inv_body = vsubq_f32(one, kick->body);

    // More attack and less body increases ratio complexity.
    // High body moves the tone toward a cleaner / heavier fundamental.
    // Approx range: 1.05 .. 3.10
    kick->mod_ratio = vaddq_f32(vdupq_n_f32(1.05f),
                                vaddq_f32(vmulq_n_f32(kick->attack, 0.95f),
                                           vmulq_n_f32(inv_body, 1.10f)));

    // Attack gives a sharper drop. Body slightly restrains the drop so the kick
    // stays low and solid instead of becoming a tom-like chirp.
    // Widened range: 0.10 .. 2.20 octaves (was 0.45..1.63).
    kick->sweep_depth = vaddq_f32(vdupq_n_f32(0.10f),
                                  vaddq_f32(vmulq_n_f32(kick->attack, 1.70f),
                                             vmulq_n_f32(inv_body, 0.40f)));

    // Sustained body FM remains moderate. Too much sustained FM weakens the
    // fundamental and makes the kick less useful in a mix.
    kick->body_index = vaddq_f32(vdupq_n_f32(0.25f),
                                 vmulq_n_f32(kick->body, 0.90f));

    // Very short attack FM.
    kick->click_index = vaddq_f32(vdupq_n_f32(0.55f),
                                  vmulq_n_f32(kick->attack, 2.65f));

    // Body compensation and transient drive.
    kick->output_gain = vaddq_f32(vdupq_n_f32(0.58f),
                                  vmulq_n_f32(kick->body, 0.42f));

    kick->drive = vaddq_f32(vdupq_n_f32(0.10f),
                            vmulq_n_f32(kick->attack, 0.85f));
}

/**
 * Set MIDI note for kick
 */
fast_inline void kick_engine_set_note(kick_engine_t* kick,
                                      uint32x4_t voice_mask,
                                      float32x4_t midi_notes) {
    float32x4_t base_freq = fm_midi_to_freq(midi_notes, KICK_FREQ_MIN, KICK_FREQ_MAX);
    kick->carrier_freq_base = vbslq_f32(voice_mask,
                                        base_freq,
                                        kick->carrier_freq_base);

    // Reset phases for a consistent front edge.
    float32x4_t zero = vdupq_n_f32(0.0f);
    kick->carrier_phase = vbslq_f32(voice_mask, zero, kick->carrier_phase);
    kick->modulator_phase = vbslq_f32(voice_mask, zero, kick->modulator_phase);
}

/**
 * Process one sample of kick engine
 */
fast_inline float32x4_t kick_engine_process(kick_engine_t* kick,
                                            float32x4_t envelope,
                                            uint32x4_t active_mask,
                                            float32x4_t lfo_pitch_mult,
                                            float32x4_t lfo_index_add) {
    const float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI * INV_SAMPLE_RATE);
    const float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);

    float32x4_t env2 = vmulq_f32(envelope, envelope);
    float32x4_t env4 = vmulq_f32(env2, env2);
    float32x4_t env8 = vmulq_f32(env4, env4);
    // Explicit envelope domains:
    // - amp_env: overall loudness/body
    // - pitch_env: short sweep so pitch drop is less exposed
    // - index_env: shorter FM brightness than amp
    float32x4_t amp_env = envelope;
    float32x4_t pitch_env = env8;
    float32x4_t index_env = env8;

    // Pitch sweep. exp2_neon remains the main expensive operation, but the
    // sweep depth is now precomputed in update().
    float32x4_t sweep_octaves = vmulq_f32(pitch_env, kick->sweep_depth);
    float32x4_t pitch_mult = exp2_neon(sweep_octaves);

    float32x4_t carrier_freq = vmulq_f32(kick->carrier_freq_base, lfo_pitch_mult);
    carrier_freq = vmulq_f32(carrier_freq, pitch_mult);

    float32x4_t mod_freq = vmulq_f32(carrier_freq, kick->mod_ratio);

    // Phase update and wrap.
    kick->carrier_phase = vaddq_f32(kick->carrier_phase,
                                    vmulq_f32(carrier_freq, two_pi_over_sr));
    kick->modulator_phase = vaddq_f32(kick->modulator_phase,
                                      vmulq_f32(mod_freq, two_pi_over_sr));

    uint32x4_t c_wrap = vcgeq_f32(kick->carrier_phase, two_pi);
    uint32x4_t m_wrap = vcgeq_f32(kick->modulator_phase, two_pi);
    kick->carrier_phase = vbslq_f32(c_wrap,
                                    vsubq_f32(kick->carrier_phase, two_pi),
                                    kick->carrier_phase);
    kick->modulator_phase = vbslq_f32(m_wrap,
                                      vsubq_f32(kick->modulator_phase, two_pi),
                                      kick->modulator_phase);

    // FM index: body remains present; click is extremely front-loaded.
    float32x4_t body_index = vmulq_f32(env4, kick->body_index);
    float32x4_t click_index = vmulq_f32(index_env, kick->click_index);
    float32x4_t index = vaddq_f32(body_index, click_index);
    index = vaddq_f32(index, lfo_index_add);

    float32x4_t modulator = neon_sin_fast(kick->modulator_phase);
    float32x4_t modulated_phase = vaddq_f32(kick->carrier_phase,
                                            vmulq_f32(modulator, index));

    float32x4_t body = neon_sin_fast(modulated_phase);
    float32x4_t transient = vmulq_f32(modulator, vmulq_f32(env8, vdupq_n_f32(0.22f)));
    float32x4_t output = vaddq_f32(vmulq_f32(body, amp_env), transient);

    // Transient drive: stronger only at the front of the hit.
    float32x4_t drive_gain = vaddq_f32(vdupq_n_f32(1.0f),
                                       vmulq_f32(kick->drive, env8));
    output = vmulq_f32(output, drive_gain);
    output = fm_cubic_clip(output);

    // Body compensation.
    output = vmulq_f32(output, kick->output_gain);

    return vbslq_f32(active_mask, output, vdupq_n_f32(0.0f));
}
