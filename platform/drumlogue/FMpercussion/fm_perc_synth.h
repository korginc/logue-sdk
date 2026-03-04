/**
 *  @file fm_perc_synth.h
 *  @brief Complete FM Percussion Synthesizer
 *
 *  Integrates all components:
 *  - 4 FM engines (Kick, Snare, Metal, Perc)
 *  - Enhanced LFO system with bipolar modulation
 *  - Envelope ROM
 *  - Parameter smoothing
 *  - Preset system
 */

#pragma once

#include <arm_neon.h>
#include "kick_engine.h"
#include "snare_engine.h"
#include "metal_engine.h"
#include "perc_engine.h"
#include "lfo_enhanced.h"
#include "lfo_smoothing.h"
#include "envelope_rom.h"
#include "prng.h"
#include "midi_handler.h"
#include "fm_presets.h"

/**
 * Complete synthesizer state
 */
typedef struct {
    // FM Engines
    kick_engine_t kick;
    snare_engine_t snare;
    metal_engine_t metal;
    perc_engine_t perc;
    
    // LFO System
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
    
    // Voice activity
    uint32x4_t active_voices;
    uint32x4_t voice_triggered;  // Which voices triggered this note
    
    // Output gain
    float master_gain;
} fm_perc_synth_t;

/**
 * Initialize synthesizer
 */
fast_inline void fm_perc_synth_init(fm_perc_synth_t* synth) {
    kick_engine_init(&synth->kick);
    snare_engine_init(&synth->snare);
    metal_engine_init(&synth->metal);
    perc_engine_init(&synth->perc);
    
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
    fm_perc_synth_update_params(synth);
}

/**
 * Update all parameters from UI
 */
fast_inline void fm_perc_synth_update_params(fm_perc_synth_t* synth) {
    uint8_t* p = synth->params;
    
    // Update engines with their parameters
    float32x4_t kick_params = {
        p[4] / 100.0f,  // Kick sweep
        p[5] / 100.0f,  // Kick decay
        0, 0
    };
    kick_engine_update(&synth->kick, 
                      vdupq_n_f32(p[4] / 100.0f),
                      vdupq_n_f32(p[5] / 100.0f));
    
    snare_engine_update(&synth->snare,
                       vdupq_n_f32(p[6] / 100.0f),
                       vdupq_n_f32(p[7] / 100.0f));
    
    metal_engine_update(&synth->metal,
                       vdupq_n_f32(p[8] / 100.0f),
                       vdupq_n_f32(p[9] / 100.0f));
    
    perc_engine_update(&synth->perc,
                      vdupq_n_f32(p[10] / 100.0f),
                      vdupq_n_f32(p[11] / 100.0f));
    
    // Update LFO with smoothing
    uint32x4_t all_voices = vdupq_n_u32(0xFFFFFFFF);
    
    // Convert depth from stored 0-200 to -100..100
    int8_t depth1 = (int8_t)(p[15] - 100);
    int8_t depth2 = (int8_t)(p[19] - 100);
    
    lfo_smoother_set_rate(&synth->lfo_smooth, 0, p[13] / 100.0f, all_voices);
    lfo_smoother_set_rate(&synth->lfo_smooth, 1, p[17] / 100.0f, all_voices);
    lfo_smoother_set_depth(&synth->lfo_smooth, 0, depth1 / 100.0f, all_voices);
    lfo_smoother_set_depth(&synth->lfo_smooth, 1, depth2 / 100.0f, all_voices);
    lfo_smoother_set_target(&synth->lfo_smooth, 0, p[14], all_voices);
    lfo_smoother_set_target(&synth->lfo_smooth, 1, p[18], all_voices);
    
    // Update envelope shape
    synth->current_env_shape = p[20];
}

/**
 * MIDI Note On handler
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
        mask |= 1 << triggers[i].voice;
        
        // Set note for specific engine
        float32x4_t midi_note = vdupq_n_f32(triggers[i].note);
        uint32x4_t voice_mask = vdupq_n_u32(1 << triggers[i].voice);
        
        switch (triggers[i].voice) {
            case 0: kick_engine_set_note(&synth->kick, voice_mask, midi_note); break;
            case 1: snare_engine_set_note(&synth->snare, voice_mask, midi_note); break;
            case 2: metal_engine_set_note(&synth->metal, voice_mask, midi_note); break;
            case 3: perc_engine_set_note(&synth->perc, voice_mask, midi_note); break;
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
 * Process one audio sample
 */
fast_inline float fm_perc_synth_process(fm_perc_synth_t* synth) {
    // Process parameter smoothing
    lfo_smoother_process(&synth->lfo_smooth);
    
    // Update LFO with smoothed values
    lfo_enhanced_update(&synth->lfo,
                       synth->params[12],  // shape combo
                       vgetq_lane_u32(synth->lfo_smooth.current_target1, 0),
                       vgetq_lane_u32(synth->lfo_smooth.current_target2, 0),
                       vgetq_lane_f32(synth->lfo_smooth.current_depth1, 0) * 100.0f,
                       vgetq_lane_f32(synth->lfo_smooth.current_depth2, 0) * 100.0f,
                       vgetq_lane_f32(synth->lfo_smooth.current_rate1, 0) * 100.0f,
                       vgetq_lane_f32(synth->lfo_smooth.current_rate2, 0) * 100.0f);
    
    // Process envelope
    neon_envelope_process(&synth->envelope);
    
    // Process LFOs
    float32x4_t lfo1, lfo2;
    lfo_enhanced_process(&synth->lfo, &lfo1, &lfo2);
    
    // Apply LFO modulations to envelope
    float32x4_t env = synth->envelope.level;
    
    if (synth->lfo.target1 == LFO_TARGET_ENV) {
        env = lfo_apply_modulation(env, lfo1, synth->lfo.depth1, 0.0f, 1.0f);
    }
    if (synth->lfo.target2 == LFO_TARGET_ENV) {
        env = lfo_apply_modulation(env, lfo2, synth->lfo.depth2, 0.0f, 1.0f);
    }
    
    // Process each engine
    float32x4_t kick_out = kick_engine_process(&synth->kick, env,
                                               synth->voice_triggered);
    float32x4_t snare_out = snare_engine_process(&synth->snare, env,
                                                  synth->voice_triggered);
    float32x4_t metal_out = metal_engine_process(&synth->metal, env,
                                                  synth->voice_triggered);
    float32x4_t perc_out = perc_engine_process(&synth->perc, env,
                                                synth->voice_triggered);
    
    // Mix and apply gain
    float32x4_t mixed = vaddq_f32(kick_out, snare_out);
    mixed = vaddq_f32(mixed, metal_out);
    mixed = vaddq_f32(mixed, perc_out);
    
    // Sum to mono (for now) and apply master gain
    float sum = vgetq_lane_f32(mixed, 0) +
                vgetq_lane_f32(mixed, 1) +
                vgetq_lane_f32(mixed, 2) +
                vgetq_lane_f32(mixed, 3);
    
    return sum * synth->master_gain;
}