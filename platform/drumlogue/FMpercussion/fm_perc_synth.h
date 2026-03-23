#pragma once

/**
 * @file fm_perc_synth.h
 * @brief FM Percussion Synth - 4 voices, 5 instruments, one instance per instrument
 *
 * Features:
 * - 5 instruments: Kick, Snare, Metal, Perc, Resonant
 * - 4 voices, each with unique instrument (no duplicates)
 * - 12 valid allocations encoded in VoiceAlloc param
 * - Per-voice probability triggering
 * - Resonant morph parameter for expressive control
 * - Enhanced LFO system with bipolar modulation
 * - Envelope ROM
 * - Parameter smoothing
 * - Preset system
 */

#include <arm_neon.h>
#include "constants.h"
#include "kick_engine.h"
#include "snare_engine.h"
#include "metal_engine.h"
#include "perc_engine.h"
#include "resonant_synthesis.h"
#include "lfo_enhanced.h"
#include "lfo_smoothing.h"
#include "envelope_rom.h"
#include "prng.h"
#include "midi_handler.h"
#include "fm_presets.h"

// Voice allocation table - 12 combinations (no duplicates)
// Format: [voice0, voice1, voice2, voice3] engine assignments
static const uint8_t VOICE_ALLOC_TABLE[VOICE_ALLOC_COUNT][VOICE_ALLOC_MAX] = {
    {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC},     // 0: K-S-M-P (no resonant)
    {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_RESONANT}, // 1: K-S-M-R
    {ENGINE_KICK, ENGINE_SNARE, ENGINE_RESONANT, ENGINE_PERC},  // 2: K-S-R-P
    {ENGINE_KICK, ENGINE_RESONANT, ENGINE_METAL, ENGINE_PERC},  // 3: K-R-M-P
    {ENGINE_RESONANT, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}, // 4: R-S-M-P
    {ENGINE_KICK, ENGINE_SNARE, ENGINE_RESONANT, ENGINE_METAL}, // 5: K-S-R-M
    {ENGINE_KICK, ENGINE_RESONANT, ENGINE_SNARE, ENGINE_PERC},  // 6: K-R-S-P
    {ENGINE_RESONANT, ENGINE_KICK, ENGINE_METAL, ENGINE_PERC},  // 7: R-K-M-P
    {ENGINE_RESONANT, ENGINE_SNARE, ENGINE_KICK, ENGINE_PERC},  // 8: R-S-K-P
    {ENGINE_METAL, ENGINE_RESONANT, ENGINE_KICK, ENGINE_PERC},  // 9: M-R-K-P
    {ENGINE_PERC, ENGINE_RESONANT, ENGINE_KICK, ENGINE_METAL},  // 10: P-R-K-M
    {ENGINE_METAL, ENGINE_PERC, ENGINE_RESONANT, ENGINE_KICK}   // 11: M-P-R-K
};

/**
 * Complete synthesizer state
 */
typedef struct {
    // FM Engines (5 total)
    kick_engine_t kick;
    snare_engine_t snare;
    metal_engine_t metal;
    perc_engine_t perc;
    resonant_synth_t resonant;

    // LFO System
    lfo_enhanced_t lfo;
    lfo_smoother_t lfo_smooth;

    // Envelope
    neon_envelope_t envelope;
    uint8_t current_env_shape;

    // Probability PRNG (4 independent streams)
    neon_prng_t prng;

    // MIDI handler
    midi_handler_t midi;

    // Current parameters (cached)
    uint8_t params[PARAM_TOTAL];

    // Voice allocation
    uint8_t voice_engine[VOICE_ALLOC_MAX];        // Engine type for each voice (0-4)
    uint8_t allocation_idx;         // Current allocation (0-11)

    // Masks for efficient NEON processing
    uint32x4_t engine_mask[ENGINE_COUNT];

    // Voice activity and probabilities
    float32x4_t voice_active;
    uint32x4_t voice_triggered;
    uint32_t voice_probs[VOICE_ALLOC_MAX];         // Per-voice probabilities (0-100)

    // Resonant morph parameter
    float resonant_morph;            // 0-1

    // Per-voice velocity (set on note-on, persists until next trigger)
    float32x4_t voice_velocity;      // 0-1 per lane

    // Output gain
    float master_gain;
} fm_perc_synth_t;

/**
 * Update voice allocation from param 21 (0-11)
 */
fast_inline void update_voice_allocation(fm_perc_synth_t* synth, uint8_t alloc_idx) {
    if (alloc_idx >= 12) alloc_idx = 0;

    synth->allocation_idx = alloc_idx;

    // Copy allocation to voice_engine array
    for (int v = 0; v < VOICE_ALLOC_MAX; v++) {
        synth->voice_engine[v] = VOICE_ALLOC_TABLE[alloc_idx][v];
    }

    // Build per-lane NEON masks for each engine type.
    // Each lane gets 0xFFFFFFFF if that voice uses this engine, 0 otherwise.
    for (int e = 0; e < ENGINE_COUNT; e++) {
        uint32_t lanes[NEON_LANES];
        for (int v = 0; v < VOICE_ALLOC_MAX; v++) {
            lanes[v] = (synth->voice_engine[v] == e) ? 0xFFFFFFFFU : 0U;
        }
        synth->engine_mask[e] = vld1q_u32(lanes);
    }
}

/**
 * Apply resonant morph parameter - controls multiple dimensions
 */
fast_inline void apply_resonant_morph(resonant_synth_t * res, float morph, uint8_t mode) {
  uint32x4_t all_voices = vdupq_n_u32(0xFFFFFFFF);

  // Morph zones with different behaviors based on mode
  switch (mode) {
    case 0:  // LowPass - morph controls cutoff frequency
    {
      float fc = 50.0f + morph * 7950.0f;  // 50-8000 Hz
      resonant_synth_set_center(res, all_voices, vdupq_n_f32(fc));
      resonant_synth_set_resonance(res, all_voices, 50.0f);  // Fixed resonance
    } break;

    case 1:  // BandPass - morph controls Q/resonance
    {
      float fc = 1000.0f;                       // Fixed center
      float resonance = 10.0f + morph * 80.0f;  // 10-90%
      resonant_synth_set_center(res, all_voices, vdupq_n_f32(fc));
      resonant_synth_set_resonance(res, all_voices, resonance);
    } break;

    case 2:  // HighPass - morph controls cutoff with inverse curve
    {
      float fc = 8000.0f - morph * 7950.0f;  // 8000-50 Hz (inverse)
      resonant_synth_set_center(res, all_voices, vdupq_n_f32(fc));
      resonant_synth_set_resonance(res, all_voices, 30.0f);
    } break;

    case 3:  // Notch - morph controls notch sharpness
    {
      // Higher resonance 'a' = narrower notch (resonance IS the width control)
      float fc = 1000.0f;
      resonant_synth_set_center(res, all_voices, vdupq_n_f32(fc));
      resonant_synth_set_resonance(res, all_voices, 20.0f + morph * 60.0f);
    } break;

    case 4:  // Peak - morph controls both frequency and gain
    {
      float fc = 200.0f + morph * 3800.0f;  // 200-4000 Hz
      float gain = 1.0f + morph * 3.0f;     // 1-4x gain
      resonant_synth_set_center(res, all_voices, vdupq_n_f32(fc));
      resonant_synth_set_resonance(res, all_voices, 30.0f + morph * 60.0f);
      res->gain = vdupq_n_f32(gain);
    } break;
  }
}

/**
 * Update all parameters from UI
 */
fast_inline void fm_perc_synth_update_params(fm_perc_synth_t* synth) {
    uint8_t* p = synth->params;

    // =================================================================
    // Update voice probabilities (Page 1, params 0-3)
    // =================================================================
    synth->voice_probs[PARAM_VOICE1_PROB] = p[PARAM_VOICE1_PROB];
    synth->voice_probs[PARAM_VOICE2_PROB] = p[PARAM_VOICE2_PROB];
    synth->voice_probs[PARAM_VOICE3_PROB] = p[PARAM_VOICE3_PROB];
    synth->voice_probs[PARAM_VOICE4_PROB] = p[PARAM_VOICE4_PROB];

    // =================================================================
    // Check if voice allocation changed (param 21)
    // =================================================================
    static uint8_t last_alloc = 0xFF;
    if (p[PARAM_VOICE_ALLOC] != last_alloc) {
    update_voice_allocation(synth, p[PARAM_VOICE_ALLOC]);
    last_alloc = p[PARAM_VOICE_ALLOC];
    }

    // =================================================================
    // Update resonant morph (param 23)
    // =================================================================
    synth->resonant_morph = p[PARAM_RES_MORPH] / 100.0f;

    // Apply morph to resonant parameters
    // Morph controls multiple parameters: mode, frequency, resonance
    apply_resonant_morph(&synth->resonant, synth->resonant_morph, p[PARAM_RES_MODE]);

    // =================================================================
    // Update FM engines (always update all - they'll be used based on allocation)
    // =================================================================

    // Kick engine: param1 = sweep depth (0-1), param2 = decay shape (0-1)
    kick_engine_update(&synth->kick,
                        vdupq_n_f32(p[PARAM_KICK_SWEEP] / 100.0f),   // Kick sweep
                        vdupq_n_f32(p[PARAM_KICK_DECAY] / 100.0f));  // Kick decay

    // Snare engine: param1 = noise mix (0-1), param2 = body resonance (0-1)
    snare_engine_update(&synth->snare,
                        vdupq_n_f32(p[PARAM_SNARE_NOISE] / 100.0f),  // Snare noise mix
                        vdupq_n_f32(p[PARAM_SNARE_BODY] / 100.0f));  // Snare body resonance

    // Metal engine: param1 = inharmonicity (0-1), param2 = brightness (0-1)
    metal_engine_update(&synth->metal,
                        vdupq_n_f32(p[PARAM_METAL_INHARM] / 100.0f),   // Metal inharmonicity
                        vdupq_n_f32(p[PARAM_METAL_BRIGHT] / 100.0f));  // Metal brightness

    // Perc engine: param1 = ratio center (0-1), param2 = variation (0-1)
    perc_engine_update(&synth->perc,
                        vdupq_n_f32(p[PARAM_PERC_RATIO] / 100.0f),  // Perc ratio center
                        vdupq_n_f32(p[PARAM_PERC_VAR] / 100.0f));   // Perc variation

    // =================================================================
    // Update resonant base parameters (mode from param 22)
    // =================================================================
    resonant_synth_set_mode(&synth->resonant,
                            vdupq_n_u32(0xFFFFFFFF),
                            (resonant_mode_t)(p[PARAM_RES_MODE] % 5));

    // =================================================================
    // Update LFO (params 12-19)
    // =================================================================
    uint32x4_t all_voices = vdupq_n_u32(0xFFFFFFFF);
    int8_t depth1 = (int8_t)(p[PARAM_LFO1_DEPTH] - 100);
    int8_t depth2 = (int8_t)(p[PARAM_LFO2_DEPTH] - 100);

    lfo_smoother_set_rate(&synth->lfo_smooth, 0, p[PARAM_LFO1_RATE] / 100.0f, all_voices);
    lfo_smoother_set_rate(&synth->lfo_smooth, 1, p[PARAM_LFO2_RATE] / 100.0f, all_voices);
    lfo_smoother_set_depth(&synth->lfo_smooth, 0, depth1 / 100.0f, all_voices);
    lfo_smoother_set_depth(&synth->lfo_smooth, 1, depth2 / 100.0f, all_voices);
    lfo_smoother_set_target(&synth->lfo_smooth, 0, p[PARAM_LFO1_TARGET], all_voices);
    lfo_smoother_set_target(&synth->lfo_smooth, 1, p[PARAM_LFO2_TARGET], all_voices);

    // =================================================================
    // Update envelope shape (param 20)
    // =================================================================
    synth->current_env_shape = p[PARAM_ENV_SHAPE];
}

/**
 * standalone function similar to load_preset
 */
fast_inline void load_fm_preset(uint8_t idx, uint8_t * params) {
    if (idx >= NUM_OF_PRESETS) return;

    const fm_preset_t * p = &FM_PRESETS[idx];

    // Page 1
    params[PARAM_VOICE1_PROB] = p->prob_kick;
    params[PARAM_VOICE2_PROB] = p->prob_snare;
    params[PARAM_VOICE3_PROB] = p->prob_metal;
    params[PARAM_VOICE4_PROB] = p->prob_perc;

    // Page 2
    params[PARAM_KICK_SWEEP] = p->kick_sweep;
    params[PARAM_KICK_DECAY] = p->kick_decay;
    params[PARAM_SNARE_NOISE] = p->snare_noise;
    params[PARAM_SNARE_BODY] = p->snare_body;

    // Page 3
    params[PARAM_METAL_INHARM] = p->metal_inharm;
    params[PARAM_METAL_BRIGHT] = p->metal_bright;
    params[PARAM_PERC_RATIO] = p->perc_ratio;
    params[PARAM_PERC_VAR] = p->perc_var;

    // Page 4 (LFO1)
    params[PARAM_LFO1_SHAPE] = p->lfo1_shape;
    params[PARAM_LFO1_RATE] = p->lfo1_rate;
    params[PARAM_LFO1_TARGET] = p->lfo1_target;
    params[PARAM_LFO1_DEPTH] = (uint8_t)(p->lfo1_depth + 100);  // Convert -100..100 to 0..200

    // Page 5 (LFO2)
    params[PARAM_LFO2_SHAPE] = p->lfo2_shape;
    params[PARAM_LFO2_RATE] = p->lfo2_rate;
    params[PARAM_LFO2_TARGET] = p->lfo2_target;
    params[PARAM_LFO2_DEPTH] = (uint8_t)(p->lfo2_depth + 100);

    // Page 6
    params[PARAM_ENV_SHAPE] = p->env_shape;
    params[PARAM_VOICE_ALLOC] = p->voice_index;
    params[PARAM_RES_MODE] = p->resonant_mode;
    params[PARAM_RES_MORPH] = p->resonant_morph;
}

/**
* Initialize synthesizer
*/
fast_inline void fm_perc_synth_init(fm_perc_synth_t * synth) {
    // Initialize all engines
    kick_engine_init(&synth->kick);
    snare_engine_init(&synth->snare);
    metal_engine_init(&synth->metal);
    perc_engine_init(&synth->perc);
    resonant_synth_init(&synth->resonant);

    // Initialize LFO and envelope
    lfo_enhanced_init(&synth->lfo);
    lfo_smoother_init(&synth->lfo_smooth);
    neon_envelope_init(&synth->envelope);

    // Initialize PRNG and MIDI
    neon_prng_init(&synth->prng, RAND_DEFAULT_SEED);
    midi_handler_init(&synth->midi);

    // Initialize parameters
    synth->voice_active = vdupq_n_f32(0.0f);
    synth->voice_triggered = vdupq_n_u32(0);
    synth->voice_velocity = vdupq_n_f32(1.0f);
    synth->master_gain = 0.25f;
    synth->current_env_shape = 40;
    synth->resonant_morph = 0.5f;

    // Default probabilities (all 100%)
    for (int i = 0; i < VOICE_ALLOC_MAX; i++) {
      synth->voice_probs[i] = 100;
    }

    // Load default preset
    load_fm_preset(0, synth->params);

    // Update voice allocation from params
    fm_perc_synth_update_params(synth);
}

/**
 * Fast NEON horizontal sum of 4 floats
 * Returns sum of all 4 lanes
 */
fast_inline float neon_horizontal_sum(float32x4_t v) {
    // Step 1: Pairwise add low and high halves
    float32x2_t sum_low = vpadd_f32(vget_low_f32(v), vget_high_f32(v));

    // Step 2: Pairwise add again to get final sum
    float32x2_t sum_total = vpadd_f32(sum_low, sum_low);

    // Step 3: Extract result
    return vget_lane_f32(sum_total, 0);
}

/**
 * Alternative method using vaddvq_f32 for ARMv8/AArch64
 * For ARMv7 (drumlogue), use the vpadd method above
 */
fast_inline float neon_horizontal_sum_alt(float32x4_t v) {
    #if defined(__aarch64__)
    // ARMv8/AArch64 has dedicated instruction
    return vaddvq_f32(v);
    #else
    // ARMv7 fallback
    float32x2_t sum_low = vpadd_f32(vget_low_f32(v), vget_high_f32(v));
    float32x2_t sum_total = vpadd_f32(sum_low, sum_low);
    return vget_lane_f32(sum_total, 0);
    #endif
}

/**
 * MIDI Note On handler with per-voice probability
 */
fast_inline void fm_perc_synth_note_on(fm_perc_synth_t* synth,
                                       uint8_t note,
                                       uint8_t velocity) {
    // Generate probability gate for all 4 voices at once
    uint32x4_t gate = probability_gate_neon(&synth->prng,
                                            synth->voice_probs[PARAM_VOICE1_PROB],
                                            synth->voice_probs[PARAM_VOICE2_PROB],
                                            synth->voice_probs[PARAM_VOICE3_PROB],
                                            synth->voice_probs[PARAM_VOICE4_PROB]);

    // Extract which voices triggered
    uint32_t gate_bits = 0;
    gate_bits |= (vgetq_lane_u32(gate, 0) ? 1 : 0) << 0;
    gate_bits |= (vgetq_lane_u32(gate, 1) ? 1 : 0) << 1;
    gate_bits |= (vgetq_lane_u32(gate, 2) ? 1 : 0) << 2;
    gate_bits |= (vgetq_lane_u32(gate, 3) ? 1 : 0) << 3;

    if (gate_bits == 0) return;  // No voices triggered

    // Store per-voice velocity (0-1) for triggered voices
    float vel_scale = velocity / 127.0f;
    float32x4_t new_velocities = vdupq_n_f32(vel_scale);
    // Use the 'gate' mask from line 310 to conditionally update lanes.
    synth->voice_velocity = vbslq_f32(gate, new_velocities, synth->voice_velocity);

    // Build voice mask from gate results
    uint32_t mask = gate_bits;

    // Set note for each triggered voice based on its engine assignment
    float32x4_t midi_note = vdupq_n_f32(note);

    for (int v = 0; v < VOICE_ALLOC_MAX; v++) {
        if (gate_bits & (1 << v)) {
            // Per-lane mask: 0xFFFFFFFF for the triggered voice lane, 0 for others
            // comparing a vector containing the current voice index v with a constant vector of indices {0, 1, 2, 3} by means of compound literal
            uint32x4_t voice_mask = vceqq_u32(vdupq_n_u32(v), vld1q_u32((const uint32_t[]){0, 1, 2, 3}));
            uint8_t engine = synth->voice_engine[v];

            switch (engine) {
                case ENGINE_KICK:
                    kick_engine_set_note(&synth->kick, voice_mask, midi_note);
                    break;
                case ENGINE_SNARE:
                    snare_engine_set_note(&synth->snare, voice_mask, midi_note);
                    break;
                case ENGINE_METAL:
                    metal_engine_set_note(&synth->metal, voice_mask, midi_note);
                    break;
                case ENGINE_PERC:
                    perc_engine_set_note(&synth->perc, voice_mask, midi_note);
                    break;
                case ENGINE_RESONANT:
                    resonant_synth_set_f0(&synth->resonant, voice_mask, midi_note);
                    break;
            }
        }
    }

    // Trigger envelope for active voices
    synth->voice_triggered = vdupq_n_u32(mask);
    neon_envelope_trigger(&synth->envelope,
                         vdupq_n_u32(mask),
                         synth->current_env_shape);
}

/**
 * MIDI Note Off handler
 */
fast_inline void fm_perc_synth_note_off(fm_perc_synth_t* synth, uint8_t note) {
    uint8_t releasing[4];
    uint32_t num_releasing = midi_note_off(&synth->midi, note, releasing);

    // For now, envelope release is triggered when any voice releases
    // In a more sophisticated version, we'd track per-voice release
    if (num_releasing > 0) {
        // Signal release stage
        // This would need per-voice release tracking
    }
}

/**
  * Process one audio sample with full LFO modulation support
 */
fast_inline float fm_perc_synth_process(fm_perc_synth_t* synth) {
    // =================================================================
    // Process parameter smoothing and LFO
    // =================================================================
    lfo_smoother_process(&synth->lfo_smooth);

    lfo_enhanced_update(&synth->lfo,
                        synth->params[PARAM_LFO1_SHAPE],
                        vgetq_lane_u32(synth->lfo_smooth.current_target1, 0),
                        vgetq_lane_u32(synth->lfo_smooth.current_target2, 0),
                        vgetq_lane_f32(synth->lfo_smooth.current_depth1, 0) * 100.0f,
                        vgetq_lane_f32(synth->lfo_smooth.current_depth2, 0) * 100.0f,
                        vgetq_lane_f32(synth->lfo_smooth.current_rate1, 0) * 100.0f,
                        vgetq_lane_f32(synth->lfo_smooth.current_rate2, 0) * 100.0f);

    // =================================================================
    // Process envelope
    // =================================================================
    neon_envelope_process(&synth->envelope);
    float32x4_t env = synth->envelope.level;

    // =================================================================
    // Process LFOs
    // =================================================================
    float32x4_t lfo1, lfo2;
    lfo_enhanced_process(&synth->lfo, &lfo1, &lfo2);

    // =================================================================
    // Apply LFO modulations to envelope (target 3)
    // =================================================================
    if (synth->lfo.target1 == LFO_TARGET_ENV || synth->lfo.target2 == LFO_TARGET_ENV) {
        float32x4_t mod = (synth->lfo.target1 == LFO_TARGET_ENV) ? lfo1 : lfo2;
        float32x4_t depth = (synth->lfo.target1 == LFO_TARGET_ENV) ? synth->lfo.depth1 : synth->lfo.depth2;
        env = lfo_apply_modulation(env, mod, depth, 0.0f, 1.0f);
    }

    // =================================================================
    // NEW: Apply LFO modulation to resonant frequency (target 6)
    // =================================================================
    if (synth->lfo.target1 == LFO_TARGET_RES_FREQ ||
        synth->lfo.target2 == LFO_TARGET_RES_FREQ) {

        // Determine which LFO is modulating resonant frequency
        float32x4_t mod_freq;
        float32x4_t depth_freq;
        uint32x4_t voice_mask;

        if (synth->lfo.target1 == LFO_TARGET_RES_FREQ) {
            mod_freq = lfo1;
            depth_freq = synth->lfo.depth1;
            voice_mask = synth->engine_mask[ENGINE_RESONANT];
        } else {
            mod_freq = lfo2;
            depth_freq = synth->lfo.depth2;
            voice_mask = synth->engine_mask[ENGINE_RESONANT];
        }

        // Get current center frequency from resonant engine.
        // Scaling freq_mod proportionally to current_fc keeps LFO depth
        // musically consistent regardless of where the morph parameter sits:
        // depth=1 always sweeps ±50% of the morph-set center frequency.
        float32x4_t current_fc = resonant_synth_get_center(&synth->resonant);

        // Convert LFO (0-1) to bipolar (-1 to 1) and scale by depth
        float32x4_t lfo_bipolar = vsubq_f32(vmulq_f32(mod_freq, vdupq_n_f32(2.0f)),
                                            vdupq_n_f32(1.0f));
        // freq_mod = bipolar * depth * (current_fc * 0.5)
        // → depth=1 swings ±50% of fc; depth=0.1 swings ±5%
        float32x4_t half_fc = vmulq_f32(current_fc, vdupq_n_f32(0.5f));
        float32x4_t freq_mod = vmulq_f32(vmulq_f32(lfo_bipolar, depth_freq), half_fc);

        // Apply to resonant engine
        resonant_synth_apply_lfo_freq(&synth->resonant, voice_mask, freq_mod);
    }

    // =================================================================
    // NEW: Apply LFO modulation to resonance amount (target 7)
    // =================================================================
    if (synth->lfo.target1 == LFO_TARGET_RESONANCE ||
        synth->lfo.target2 == LFO_TARGET_RESONANCE) {

        // Determine which LFO is modulating resonance
        float32x4_t mod_res;
        float32x4_t depth_res;
        uint32x4_t voice_mask;

        if (synth->lfo.target1 == LFO_TARGET_RESONANCE) {
            mod_res = lfo1;
            depth_res = synth->lfo.depth1;
            voice_mask = synth->engine_mask[ENGINE_RESONANT];
        } else {
            mod_res = lfo2;
            depth_res = synth->lfo.depth2;
            voice_mask = synth->engine_mask[ENGINE_RESONANT];
        }

        // Get current resonance from resonant engine.
        // Scaling by the symmetric available headroom makes depth=1 sweep
        // the full range in both directions without hard-clipping:
        // swing = min(current_res, 0.99 - current_res)  →  always balanced.
        float32x4_t current_res = resonant_synth_get_resonance(&synth->resonant);

        float32x4_t lfo_bipolar = vsubq_f32(vmulq_f32(mod_res, vdupq_n_f32(2.0f)),
                                            vdupq_n_f32(1.0f));
        // Symmetric available swing: whichever limit (0 or 0.99) is closer
        float32x4_t headroom = vsubq_f32(vdupq_n_f32(0.99f), current_res);
        float32x4_t swing    = vminq_f32(current_res, headroom);
        // res_mod = bipolar * depth * swing → depth=1 sweeps symmetrically to the nearest limit
        float32x4_t bipolar_depth = vmulq_f32(lfo_bipolar, depth_res);
        float32x4_t res_mod = vmulq_f32(bipolar_depth, swing);

        // Apply to resonant engine
        resonant_synth_apply_lfo_resonance(&synth->resonant, voice_mask, res_mod);
    }

    // =================================================================
    // Process each engine with its voice mask
    // =================================================================
    float32x4_t kick_out = kick_engine_process(&synth->kick, env,
                                               synth->engine_mask[ENGINE_KICK]);
    float32x4_t snare_out = snare_engine_process(&synth->snare, env,
                                                  synth->engine_mask[ENGINE_SNARE]);
    float32x4_t metal_out = metal_engine_process(&synth->metal, env,
                                                  synth->engine_mask[ENGINE_METAL]);
    float32x4_t perc_out = perc_engine_process(&synth->perc, env,
                                                synth->engine_mask[ENGINE_PERC]);
    float32x4_t resonant_out = resonant_synth_process(&synth->resonant,
                                                       synth->engine_mask[ENGINE_RESONANT]);

    // =================================================================
    // Mix all engines, then apply per-voice velocity scaling
    // =================================================================
    float32x4_t mixed = vaddq_f32(kick_out, snare_out);
    mixed = vaddq_f32(mixed, metal_out);
    mixed = vaddq_f32(mixed, perc_out);
    mixed = vaddq_f32(mixed, resonant_out);

    // Scale each voice lane by its stored velocity before summing
    mixed = vmulq_f32(mixed, synth->voice_velocity);


    // =================================================================
    // FIXED: Efficient horizontal sum using NEON vpadd
    // =================================================================
    float sum = neon_horizontal_sum(mixed);

    // Apply master gain
    return sum * synth->master_gain;
}
