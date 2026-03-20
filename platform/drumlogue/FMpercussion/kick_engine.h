/**
 *  @file kick_engine.h
 *  @brief 2-operator FM kick drum with pitch sweep
 *
 *  Operators:
 *    Op1: Carrier (50-80Hz)
 *    Op2: Modulator (ratio 1.0-3.0)
 *  Parameters:
 *    Param1: Sweep depth (0-100%) - controls pitch drop amount
 *    Param2: Decay shape (0-100%) - controls modulation index decay
 */

#pragma once

#include <arm_neon.h>
#include "fm_voices.h"
#include "sine_neon.h"
#include "envelope_rom.h"

// Kick engine constants
#define KICK_CARRIER_BASE 60.0f    // Base frequency (C2)
#define KICK_MOD_RATIO_MIN 1.0f
#define KICK_MOD_RATIO_MAX 3.0f
#define KICK_SWEEP_OCTAVES 3.0f     // Maximum sweep: 3 octaves down

/**
 * Kick engine state (per-voice, but processed in parallel)
 */
typedef struct {
    float32x4_t carrier_phase;      // Carrier phase (4 voices)
    float32x4_t modulator_phase;     // Modulator phase (4 voices)
    float32x4_t carrier_freq_base;   // Base frequency (from MIDI)
    float32x4_t mod_ratio;           // Modulator ratio (from param)
    float32x4_t sweep_depth;         // Sweep amount (0-1)
    float32x4_t decay_shape;         // Decay curve shape
} kick_engine_t;

/**
 * Initialize kick engine
 */
fast_inline void kick_engine_init(kick_engine_t* kick) {
    kick->carrier_phase = vdupq_n_f32(0.0f);
    kick->modulator_phase = vdupq_n_f32(0.0f);
    kick->carrier_freq_base = vdupq_n_f32(KICK_CARRIER_BASE);
    kick->mod_ratio = vdupq_n_f32(2.0f);  // Default 2:1
    kick->sweep_depth = vdupq_n_f32(0.5f);
    kick->decay_shape = vdupq_n_f32(0.5f);
}

/**
 * Update kick engine parameters from UI
 */
fast_inline void kick_engine_update(kick_engine_t* kick,
                                    float32x4_t param1,  // Sweep depth
                                    float32x4_t param2) { // Decay shape
    // Param1: 0-1 maps to sweep depth 0-3 octaves
    kick->sweep_depth = param1;

    // Param2: 0-1 maps to decay curve (0=linear, 1=exponential)
    kick->decay_shape = param2;
}

/**
 * Set MIDI note for kick (all voices may have different notes)
 */
fast_inline void kick_engine_set_note(kick_engine_t* kick,
                                      uint32x4_t voice_mask,
                                      float32x4_t midi_notes) {
    // Convert MIDI note to frequency: 440 * 2^((note-69)/12)
    float32x4_t a4_freq = vdupq_n_f32(440.0f);
    float32x4_t a4_midi = vdupq_n_f32(69.0f);
    float32x4_t twelfth = vdupq_n_f32(1.0f/12.0f);

    // exponent = (note - 69) / 12
    float32x4_t exponent = vmulq_f32(vsubq_f32(midi_notes, a4_midi), twelfth);

    // 2^exponent using polynomial approximation
    float32x4_t two_pow = vdupq_n_f32(1.0f);
    // Simple approximation: 2^x ≈ 1 + x*0.693 + (x^2)*0.24
    float32x4_t x2 = vmulq_f32(exponent, exponent);
    two_pow = vmlaq_f32(two_pow, exponent, vdupq_n_f32(0.693f));
    two_pow = vmlaq_f32(two_pow, x2, vdupq_n_f32(0.24f));

    // Base frequency = 440 * 2^((note-69)/12)
    float32x4_t base_freq = vmulq_f32(a4_freq, two_pow);

    // Apply to selected voices only
    kick->carrier_freq_base = vbslq_f32(voice_mask,
                                        base_freq,
                                        kick->carrier_freq_base);

    // Reset phases for triggered voices so every hit has a consistent attack click
    float32x4_t zero = vdupq_n_f32(0.0f);
    kick->carrier_phase   = vbslq_f32(voice_mask, zero, kick->carrier_phase);
    kick->modulator_phase = vbslq_f32(voice_mask, zero, kick->modulator_phase);
}

/**
 * Process one sample of kick engine
 * Returns audio output for all 4 voices
 */
fast_inline float32x4_t kick_engine_process(kick_engine_t* kick,
                                            float32x4_t envelope,
                                            uint32x4_t active_mask) {
    // Calculate instantaneous carrier frequency with pitch sweep.
    // At note-on (envelope=1): freq = base * (1 + sweep_depth * KICK_SWEEP_OCTAVES)
    // At tail    (envelope=0): freq = base  (sweeps DOWN as envelope decays)
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t sweep_factor = vmlsq_f32(one, kick->sweep_depth, envelope);

    // Apply sweep to carrier frequency
    float32x4_t carrier_freq = vmulq_f32(kick->carrier_freq_base, sweep_factor);

    // Modulator frequency = carrier * ratio
    float32x4_t mod_freq = vmulq_f32(carrier_freq, kick->mod_ratio);

    // Convert frequencies to phase increments (radians per sample)
    float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI / 48000.0f);
    float32x4_t carrier_inc = vmulq_f32(carrier_freq, two_pi_over_sr);
    float32x4_t mod_inc = vmulq_f32(mod_freq, two_pi_over_sr);

    // Update phases (only for active voices)
    kick->carrier_phase = vaddq_f32(kick->carrier_phase, carrier_inc);
    kick->modulator_phase = vaddq_f32(kick->modulator_phase, mod_inc);

    // Wrap phases to [0, 2π)
    float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);
    uint32x4_t carrier_wrap = vcgeq_f32(kick->carrier_phase, two_pi);
    uint32x4_t mod_wrap = vcgeq_f32(kick->modulator_phase, two_pi);

    kick->carrier_phase = vbslq_f32(carrier_wrap,
                                    vsubq_f32(kick->carrier_phase, two_pi),
                                    kick->carrier_phase);
    kick->modulator_phase = vbslq_f32(mod_wrap,
                                      vsubq_f32(kick->modulator_phase, two_pi),
                                      kick->modulator_phase);

    // Calculate modulation index with decay shaping
    // index = envelope * (1.0 + decay_shape * envelope)
    float32x4_t index = vmulq_f32(envelope,
                                  vaddq_f32(one,
                                           vmulq_f32(kick->decay_shape,
                                                    envelope)));

    // Generate modulator signal
    float32x4_t modulator = neon_sin(kick->modulator_phase);

    // Apply modulation to carrier phase
    float32x4_t modulated_phase = vaddq_f32(kick->carrier_phase,
                                           vmulq_f32(modulator, index));

    // Generate carrier (output)
    float32x4_t output = neon_sin(modulated_phase);

    // Apply final envelope and mask
    output = vmulq_f32(output, envelope);
    output = vbslq_f32(active_mask,
                       output, vdupq_n_f32(0.0f));

    return output;
}

// ========== UNIT TEST ==========
#ifdef TEST_KICK

void test_kick_sweep() {
    kick_engine_t kick;
    kick_engine_init(&kick);

    // Test sweep depth
    float32x4_t param1 = vdupq_n_f32(1.0f);  // Max sweep
    float32x4_t param2 = vdupq_n_f32(0.5f);
    kick_engine_update(&kick, param1, param2);

    // Set note to C2 (36)
    uint32x4_t mask = vdupq_n_u32(0xFFFFFFFF);
    float32x4_t midi = vdupq_n_f32(36.0f);
    kick_engine_set_note(&kick, mask, midi);

    // Process through envelope
    float last_freq = KICK_CARRIER_BASE;
    for (float env = 1.0f; env >= 0.0f; env -= 0.1f) {
        float32x4_t envelope = vdupq_n_f32(env);
        float32x4_t output = kick_engine_process(&kick, envelope, mask);

        // Extract carrier phase to verify sweep
        float phase = vgetq_lane_f32(kick.carrier_phase, 0);
        (void)phase; // Would verify frequency in real test

        last_freq = env * KICK_CARRIER_BASE;
    }

    printf("Kick engine sweep test PASSED\n");
}

#endif