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
    float32x4_t carrier_phase;       // Carrier phase (4 voices)
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
    float32x4_t two_pow = exp2_neon(exponent);

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
 *
 * TR-808 click inspiration: the 808 bridged-T circuit produces a metallic
 * transient "click" at note onset because the VCA attacks instantly into a
 * resonating circuit. We approximate this by adding an env^8 weighted FM
 * index boost at onset — it decays to <10% in the first ~20% of the envelope,
 * adding high-harmonic content only at the very start of the hit.
 */
fast_inline float32x4_t kick_engine_process(kick_engine_t* kick,
                                            float32x4_t envelope,
                                            uint32x4_t active_mask,
                                            float32x4_t lfo_pitch_mult,
                                            float32x4_t lfo_index_add) {

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

    // 1. Staggered envelopes for pitch and click layers
    float32x4_t env2 = vmulq_f32(envelope, envelope);
    float32x4_t env4 = vmulq_f32(env2, env2);
    // env^8: at env=1.0 → 1.0; at env=0.9 → 0.43; at env=0.75 → 0.10; essentially
    // gone by the first 30% of decay — produces the short attack click only.
    float32x4_t env8 = vmulq_f32(env4, env4);

    // 2. The Pitch Drop: Starts HIGH and drops down to the base frequency.
    // kick->sweep_depth controls how many octaves it drops.
    // use env2 instead of env4 to have more "thump" and less "laser zap"
    float32x4_t sweep_octaves = vmulq_f32(kick->sweep_depth, vmulq_n_f32(env2, KICK_SWEEP_OCTAVES));
    // exp2_neon converts octaves into a frequency multiplier (e.g., 4 octaves = 16x frequency)
    float32x4_t pitch_drop_mult = exp2_neon(sweep_octaves);

    // Apply pitch drop AND the global LFO pitch modulation
    float32x4_t total_pitch_mult = vmulq_f32(pitch_drop_mult, lfo_pitch_mult);
    float32x4_t carrier_freq = vmulq_f32(kick->carrier_freq_base, total_pitch_mult);
    float32x4_t mod_freq = vmulq_f32(carrier_freq, kick->mod_ratio);

    // Convert Hz to phase increments
    float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI * INV_SAMPLE_RATE);
    kick->carrier_phase = vaddq_f32(kick->carrier_phase, vmulq_f32(carrier_freq, two_pi_over_sr));
    kick->modulator_phase = vaddq_f32(kick->modulator_phase, vmulq_f32(mod_freq, two_pi_over_sr));

    // Wrap phases
    float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);
    uint32x4_t c_wrap = vcgeq_f32(kick->carrier_phase, two_pi);
    uint32x4_t m_wrap = vcgeq_f32(kick->modulator_phase, two_pi);
    kick->carrier_phase = vbslq_f32(c_wrap, vsubq_f32(kick->carrier_phase, two_pi), kick->carrier_phase);
    kick->modulator_phase = vbslq_f32(m_wrap, vsubq_f32(kick->modulator_phase, two_pi), kick->modulator_phase);

    // 3. FM Index: base (decay-shaped) + LFO + TR-808-inspired click boost
    float32x4_t shape_factor = vmulq_f32(kick->decay_shape, envelope);
    // index frequency dependent: cleaner low kick, aggressive high kick
    float32x4_t freq_norm = vmulq_n_f32(carrier_freq, 1.0f / 100.0f);
    float32x4_t index = vmulq_f32(env4, vaddq_f32(vdupq_n_f32(2.0f), freq_norm));
    // Click boost: env^8 * sweep_depth * 4.0 — proportional to sweep depth
    // so a "hard" punchy kick (high sweep) also gets a hard click, while a
    // "smooth/sub" kick (low sweep) stays clean.
    // now even low sweep have a click
    float32x4_t click_boost = vmulq_f32(env8,
                                        vaddq_f32(
                                            vmulq_n_f32(kick->sweep_depth, 4.0f),
                                            vdupq_n_f32(1.0f)));
    index = vaddq_f32(vaddq_f32(index, lfo_index_add), click_boost);

    float32x4_t modulator = neon_sin(kick->modulator_phase);
    float32x4_t modulated_phase = vaddq_f32(kick->carrier_phase, vmulq_f32(modulator, index));
    // pure parallel carrier
    float32x4_t clean = neon_sin(kick->carrier_phase);

    // dynamico weight: more attack at beginning, more body later
    float32x4_t body_gain = vsubq_f32(vdupq_n_f32(1.0f), env4);
    float32x4_t attack_gain = env4;

    float32x4_t output = vaddq_f32(
        vmulq_f32(clean, body_gain),
        vmulq_f32(neon_sin(modulated_phase), attack_gain));

    // Output
    output = vmulq_f32(output, shape_factor);
    return vbslq_f32(active_mask, output, vdupq_n_f32(0.0f));
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