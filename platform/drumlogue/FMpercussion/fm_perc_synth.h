#pragma once

/**
 * @file fm_perc_synth.h
 * @brief Complete FM Percussion Synthesizer with Voice Allocation
 *
 * Features:
 * - 5 engine types (Kick, Snare, Metal, Perc, Resonant)
 * - Voice allocation: one instrument per voice, one resonant instrument
 * - Updated LFO targets (0-7) including resonant parameters
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

// Engine modes
#define ENGINE_MODE_KICK     0
#define ENGINE_MODE_SNARE    1
#define ENGINE_MODE_METAL    2
#define ENGINE_MODE_PERC     3
#define ENGINE_MODE_RESONANT 4
#define ENGINE_MODE_COUNT    5

// LFO targets (updated)
#define LFO_TARGET_NONE        0
#define LFO_TARGET_PITCH       1
#define LFO_TARGET_INDEX       2
#define LFO_TARGET_ENV         3
#define LFO_TARGET_LFO2_PHASE  4
#define LFO_TARGET_LFO1_PHASE  5
#define LFO_TARGET_RES_FREQ    6  // NEW: Resonant center frequency
#define LFO_TARGET_RESONANCE   7  // NEW: Resonance amount

// Voice allocation presets (which instrument gets resonant treatment)
// Format: [kick, snare, metal, perc, resonant_voice]
static const uint8_t VOICE_ALLOC[5][5] = {
    {ENGINE_MODE_KICK, ENGINE_MODE_SNARE, ENGINE_MODE_METAL, ENGINE_MODE_PERC, 0xFF},        // 0: All standard (no resonant)
    {ENGINE_MODE_KICK, ENGINE_MODE_SNARE, ENGINE_MODE_METAL, ENGINE_MODE_RESONANT, 3},       // 1: Perc -> Resonant
    {ENGINE_MODE_KICK, ENGINE_MODE_SNARE, ENGINE_MODE_RESONANT, ENGINE_MODE_PERC, 2},        // 2: Metal -> Resonant
    {ENGINE_MODE_KICK, ENGINE_MODE_RESONANT, ENGINE_MODE_METAL, ENGINE_MODE_PERC, 1},        // 3: Snare -> Resonant
    {ENGINE_MODE_RESONANT, ENGINE_MODE_SNARE, ENGINE_MODE_METAL, ENGINE_MODE_PERC, 0}        // 4: Kick -> Resonant
};

/**
 * Complete synthesizer state
 */
typedef struct {
    // FM Engines
    kick_engine_t kick;
    snare_engine_t snare;
    metal_engine_t metal;
    perc_engine_t perc;
    resonant_synth_t resonant;

    // LFO System (updated for 8 targets)
    lfo_enhanced_t lfo;
    lfo_smoother_t lfo_smooth;

    // Envelope
    neon_envelope_t envelope;
    uint8_t current_env_shape;

    // Probability PRNG
    neon_prng_t prng;

    // MIDI handler
    midi_handler_t midi;

    // Current parameters (cached)
    uint8_t params[24];

    // Voice allocation
    uint8_t voice_engine[4];        // Engine type for each voice (0-4)
    uint8_t resonant_voice;         // Which voice is resonant (0-3 or 0xFF if none)

    // Masks for efficient NEON processing
    uint32x4_t engine_mask[ENGINE_MODE_COUNT];

    // Voice activity
    uint32x4_t active_voices;
    uint32x4_t voice_triggered;

    // Output gain
    float master_gain;
} fm_perc_synth_t;

/**
 * Update voice allocation based on param 21
 */
fast_inline void update_voice_allocation(fm_perc_synth_t* synth) {
    uint8_t alloc_idx = synth->params[21] % 5;

    // Set voice engines from allocation table
    for (int v = 0; v < 4; v++) {
        synth->voice_engine[v] = VOICE_ALLOC[alloc_idx][v];
    }

    // Store which voice is resonant (or 0xFF if none)
    synth->resonant_voice = VOICE_ALLOC[alloc_idx][4];

    // Build masks for each engine type
    for (int e = 0; e < ENGINE_MODE_COUNT; e++) {
        uint32_t mask = 0;
        for (int v = 0; v < 4; v++) {
            if (synth->voice_engine[v] == e) {
                mask |= (1 << v);
            }
        }
        synth->engine_mask[e] = vdupq_n_u32(mask);
    }
}

/**
 * Initialize synthesizer
 */
fast_inline void fm_perc_synth_init(fm_perc_synth_t* synth) {
    kick_engine_init(&synth->kick);
    snare_engine_init(&synth->snare);
    metal_engine_init(&synth->metal);
    perc_engine_init(&synth->perc);
    resonant_synth_init(&synth->resonant);

    lfo_enhanced_init(&synth->lfo);
    lfo_smoother_init(&synth->lfo_smooth);
    neon_envelope_init(&synth->envelope);

    neon_prng_init(&synth->prng, 0x12345678);
    midi_handler_init(&synth->midi);

    synth->active_voices = vdupq_n_u32(0);
    synth->voice_triggered = vdupq_n_u32(0);
    synth->master_gain = 0.25f;
    synth->current_env_shape = 40;

    // Load default preset
    load_fm_preset(0, synth->params);

    // Update voice allocation from params
    update_voice_allocation(synth);
    fm_perc_synth_update_params(synth);
}

/**
 * Update all parameters from UI
 */
fast_inline void fm_perc_synth_update_params(fm_perc_synth_t* synth) {
    uint8_t* p = synth->params;

    // Check if voice allocation changed
    static uint8_t last_alloc = 0xFF;
    if (p[21] != last_alloc) {
        update_voice_allocation(synth);
        last_alloc = p[21];
    }

    // =================================================================
    // Update all engines (they'll be used based on allocation)
    // =================================================================

    // Kick engine
    kick_engine_update(&synth->kick,
                       vdupq_n_f32(p[4] / 100.0f),
                       vdupq_n_f32(p[5] / 100.0f));

    // Snare engine
    snare_engine_update(&synth->snare,
                        vdupq_n_f32(p[6] / 100.0f),
                        vdupq_n_f32(p[7] / 100.0f));

    // Metal engine
    metal_engine_update(&synth->metal,
                        vdupq_n_f32(p[8] / 100.0f),
                        vdupq_n_f32(p[9] / 100.0f));

    // Perc engine
    perc_engine_update(&synth->perc,
                       vdupq_n_f32(p[10] / 100.0f),
                       vdupq_n_f32(p[11] / 100.0f));

    // =================================================================
    // Update resonant engine (using params 22-23)
    // =================================================================
    uint32x4_t all_voices = vdupq_n_u32(0xFFFFFFFF);

    // Resonant mode (param 22)
    resonant_synth_set_mode(&synth->resonant, all_voices,
                           (resonant_mode_t)(p[22] % 5));

    // Resonance amount (use param 2 as default - can be changed)
    resonant_synth_set_resonance(&synth->resonant, all_voices, p[2]);  // Depth param

    // Center frequency (param 23 maps 0-100 to 50-8000 Hz)
    float fc = 50.0f + (p[23] / 100.0f) * 7950.0f;
    resonant_synth_set_center(&synth->resonant, all_voices, vdupq_n_f32(fc));

    // Mix (fixed at 0.5 for now)
    resonant_synth_set_mix(&synth->resonant, all_voices, 50.0f);

    // =================================================================
    // Update LFO with new target ranges (0-7)
    // =================================================================
    int8_t depth1 = (int8_t)(p[15] - 100);
    int8_t depth2 = (int8_t)(p[19] - 100);

    lfo_smoother_set_rate(&synth->lfo_smooth, 0, p[13] / 100.0f, all_voices);
    lfo_smoother_set_rate(&synth->lfo_smooth, 1, p[17] / 100.0f, all_voices);
    lfo_smoother_set_depth(&synth->lfo_smooth, 0, depth1 / 100.0f, all_voices);
    lfo_smoother_set_depth(&synth->lfo_smooth, 1, depth2 / 100.0f, all_voices);
    lfo_smoother_set_target(&synth->lfo_smooth, 0, p[14], all_voices);  // 0-7
    lfo_smoother_set_target(&synth->lfo_smooth, 1, p[18], all_voices);  // 0-7

    // =================================================================
    // Update envelope shape
    // =================================================================
    synth->current_env_shape = p[20];
}

/**
 * MIDI Note On handler with voice allocation
 */
fast_inline void fm_perc_synth_note_on(fm_perc_synth_t* synth,
                                       uint8_t note,
                                       uint8_t velocity) {
    uint32_t probs[4] = {
        synth->params[0],  // Kick prob
        synth->params[1],  // Snare prob
        synth->params[2],  // Metal prob
        synth->params[3]   // Perc prob
    };

    trigger_event_t triggers[4];
    uint32_t num_triggers = midi_note_on(&synth->midi, &synth->prng,
                                         note, velocity, probs, triggers);

    // Build voice mask from triggers
    uint32_t mask = 0;

    for (uint32_t i = 0; i < num_triggers; i++) {
        uint8_t voice = triggers[i].voice;
        mask |= 1 << voice;

        // Get engine type for this voice
        uint8_t engine = synth->voice_engine[voice];

        // Set note for the appropriate engine
        float32x4_t midi_note = vdupq_n_f32(triggers[i].note);
        uint32x4_t voice_mask = vdupq_n_u32(1 << voice);

        switch (engine) {
            case ENGINE_MODE_KICK:
                kick_engine_set_note(&synth->kick, voice_mask, midi_note);
                break;
            case ENGINE_MODE_SNARE:
                snare_engine_set_note(&synth->snare, voice_mask, midi_note);
                break;
            case ENGINE_MODE_METAL:
                metal_engine_set_note(&synth->metal, voice_mask, midi_note);
                break;
            case ENGINE_MODE_PERC:
                perc_engine_set_note(&synth->perc, voice_mask, midi_note);
                break;
            case ENGINE_MODE_RESONANT:
                resonant_synth_set_f0(&synth->resonant, voice_mask, midi_note);
                break;
        }
    }

    // Trigger envelope for active voices
    if (mask) {
        synth->voice_triggered = vdupq_n_u32(mask);
        neon_envelope_trigger(&synth->envelope,
                             vdupq_n_u32(mask),
                             synth->current_env_shape);
    }
}

/**
 * Process one audio sample (optimized for engine masks)
 */
fast_inline float fm_perc_synth_process(fm_perc_synth_t* synth) {
    // =================================================================
    // Process parameter smoothing
    // =================================================================
    lfo_smoother_process(&synth->lfo_smooth);

    // =================================================================
    // Update LFO with smoothed values
    // =================================================================
    lfo_enhanced_update(&synth->lfo,
                        synth->params[12],
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
    // Apply LFO modulations (including new resonant targets)
    // =================================================================
    if (synth->lfo.target1 == LFO_TARGET_ENV) {
        env = lfo_apply_modulation(env, lfo1, synth->lfo.depth1, 0.0f, 1.0f);
    }
    if (synth->lfo.target2 == LFO_TARGET_ENV) {
        env = lfo_apply_modulation(env, lfo2, synth->lfo.depth2, 0.0f, 1.0f);
    }

    // Apply resonant parameter modulations
    if (synth->lfo.target1 == LFO_TARGET_RES_FREQ || synth->lfo.target2 == LFO_TARGET_RES_FREQ) {
        float32x4_t mod = (synth->lfo.target1 == LFO_TARGET_RES_FREQ) ? lfo1 : lfo2;
        float32x4_t depth = (synth->lfo.target1 == LFO_TARGET_RES_FREQ) ? synth->lfo.depth1 : synth->lfo.depth2;

        // Get current center frequency
        float fc = vgetq_lane_f32(synth->resonant.target_fc, 0);
        float32x4_t fc_mod = lfo_apply_modulation(vdupq_n_f32(fc), mod, depth,
                                                   RESONANT_CENTER_FREQ_MIN,
                                                   RESONANT_CENTER_FREQ_MAX);
        resonant_synth_set_center(&synth->resonant, vdupq_n_u32(0xFFFFFFFF), fc_mod);
    }

    if (synth->lfo.target1 == LFO_TARGET_RESONANCE || synth->lfo.target2 == LFO_TARGET_RESONANCE) {
        float32x4_t mod = (synth->lfo.target1 == LFO_TARGET_RESONANCE) ? lfo1 : lfo2;
        float32x4_t depth = (synth->lfo.target1 == LFO_TARGET_RESONANCE) ? synth->lfo.depth1 : synth->lfo.depth2;

        // Get current resonance
        float res = vgetq_lane_f32(synth->resonant.target_resonance, 0);
        float32x4_t res_mod = lfo_apply_modulation(vdupq_n_f32(res), mod, depth,
                                                    RESONANT_RESONANCE_MIN,
                                                    RESONANT_RESONANCE_MAX);
        resonant_synth_set_resonance(&synth->resonant, vdupq_n_u32(0xFFFFFFFF),
                                     res_mod * 100.0f);  // Convert to percent
    }

    // =================================================================
    // Process each engine with its voice mask
    // =================================================================
    float32x4_t kick_out = kick_engine_process(&synth->kick, env,
                                               synth->engine_mask[ENGINE_MODE_KICK]);
    float32x4_t snare_out = snare_engine_process(&synth->snare, env,
                                                  synth->engine_mask[ENGINE_MODE_SNARE]);
    float32x4_t metal_out = metal_engine_process(&synth->metal, env,
                                                  synth->engine_mask[ENGINE_MODE_METAL]);
    float32x4_t perc_out = perc_engine_process(&synth->perc, env,
                                                synth->engine_mask[ENGINE_MODE_PERC]);
    float32x4_t resonant_out = resonant_synth_process(&synth->resonant,
                                                       synth->engine_mask[ENGINE_MODE_RESONANT]);

    // =================================================================
    // Mix all engines
    // =================================================================
    float32x4_t mixed = vaddq_f32(kick_out, snare_out);
    mixed = vaddq_f32(mixed, metal_out);
    mixed = vaddq_f32(mixed, perc_out);
    mixed = vaddq_f32(mixed, resonant_out);

    // Efficient horizontal sum
    float sum = neon_horizontal_sum(mixed);

    return sum * synth->master_gain;
}

/**
 * Get engine type for a specific voice
 */
fast_inline uint8_t fm_perc_synth_get_voice_engine(const fm_perc_synth_t* synth, uint8_t voice) {
    if (voice < 4) return synth->voice_engine[voice];
    return ENGINE_MODE_KICK;
}

/**
 * Check if a voice is using resonant engine
 */
fast_inline bool fm_perc_synth_is_resonant_voice(const fm_perc_synth_t* synth, uint8_t voice) {
    return (voice < 4) && (synth->voice_engine[voice] == ENGINE_MODE_RESONANT);
}