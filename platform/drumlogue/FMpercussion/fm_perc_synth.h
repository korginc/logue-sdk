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

// Euclidean tuning offset table
// offsets[mode][voice] = semitones above root for that voice.
// Derived from E(4,n): position[i] = floor(i * n / 4), i = 0..3.
static const float EUCLID_OFFSETS[EUCLID_MODE_COUNT][4] = {
    { 0.f,  0.f,  0.f,  0.f},  // 0: Off         — all unison
    { 0.f,  1.f,  2.f,  3.f},  // 1: E(4,4)  [0,1,2,3]  chromatic cluster
    { 0.f,  1.f,  3.f,  4.f},  // 2: E(4,6)  [0,1,3,4]  minor 3rd pairs
    { 0.f,  1.f,  3.f,  5.f},  // 3: E(4,7)  [0,1,3,5]  diatonic cluster
    { 0.f,  2.f,  4.f,  6.f},  // 4: E(4,8)  [0,2,4,6]  whole tone
    { 0.f,  2.f,  5.f,  7.f},  // 5: E(4,10) [0,2,5,7]  pentatonic/5th
    { 0.f,  3.f,  6.f,  9.f},  // 6: E(4,12) [0,3,6,9]  diminished 7th
    { 0.f,  4.f,  8.f, 12.f},  // 7: E(4,16) [0,4,8,12] augmented + octave
    { 0.f,  6.f, 12.f, 18.f},  // 8: E(4,24) [0,6,12,18] tritone spread
};

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
    int8_t params[PARAM_TOTAL];

    // Voice allocation
    uint8_t voice_engine[VOICE_ALLOC_MAX];  // Engine type for each voice (0-4)
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

    // Euclidean tuning: per-voice semitone offsets applied at note-on.
    // Loaded from EUCLID_OFFSETS[EuclTun] in update_params; [0,0,0,0] when mode=Off.
    float32x4_t euclid_offsets;

    // Output gain
    float master_gain;

    // Per-engine output band filters — keeps each engine in its natural frequency
    // range so voices don't mask each other.  Each one_pole_t holds one float32x4_t
    // state word (one state per NEON lane = per voice).
    // Precomputed alpha = 2πf/(2πf+sr) — see constants below.
    one_pole_t kick_out_lpf;   // LP 250 Hz  — keeps kick body, removes highs
    one_pole_t snare_hpf;      // LP state for HP 100 Hz subtraction — removes kick rumble
    one_pole_t snare_out_lpf;  // LP 7000 Hz — removes ultrasonic hash
    one_pole_t metal_hpf;      // LP state for HP 800 Hz subtraction — removes lows
    one_pole_t perc_hpf;       // LP state for HP 80 Hz subtraction  — removes sub rumble
    one_pole_t perc_out_lpf;   // LP 3000 Hz — removes high-frequency clash with metal
} fm_perc_synth_t;

// Precomputed one-pole filter alpha values: 2πf / (2πf + 48000)
// These are constant across the lifetime of the plugin so computing once is correct.
static const float FILT_KICK_LP_A  = 1570.796f  / (1570.796f  + 48000.0f); // 250 Hz
static const float FILT_SNARE_HP_A = 628.318f   / (628.318f   + 48000.0f); // 100 Hz
static const float FILT_SNARE_LP_A = 43982.297f / (43982.297f + 48000.0f); // 7000 Hz
static const float FILT_METAL_HP_A = 5026.548f  / (5026.548f  + 48000.0f); // 800 Hz
static const float FILT_PERC_HP_A  = 502.655f   / (502.655f   + 48000.0f); // 80 Hz
static const float FILT_PERC_LP_A  = 18849.556f / (18849.556f + 48000.0f); // 3000 Hz

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
      float fc = 50.0f + morph * 15950.0f;  // 50-16000 Hz, as morph is 0..1
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
      float fc = 16000.0f - morph * 15950.0f;  // 16000-50 Hz (inverse)
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
    int8_t* p = synth->params;

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
    synth->resonant_morph = p[PARAM_RES_MORPH] * 0.01f;

    // Apply morph to resonant parameters
    // Morph controls multiple parameters: mode, frequency, resonance
    apply_resonant_morph(&synth->resonant, synth->resonant_morph, p[PARAM_RES_MODE]);

    // =================================================================
    // Update FM engines (always update all - they'll be used based on allocation)
    // =================================================================

    // Kick engine: param1 = sweep depth (0-1), param2 = decay shape (0-1)
    kick_engine_update(&synth->kick,
                        vdupq_n_f32(p[PARAM_KICK_SWEEP] * 0.01f),   // Kick sweep
                        vdupq_n_f32(p[PARAM_KICK_DECAY] * 0.01f));  // Kick decay

    // Snare engine: param1 = noise mix (0-1), param2 = body resonance (0-1)
    snare_engine_update(&synth->snare,
                        vdupq_n_f32(p[PARAM_SNARE_NOISE] * 0.01f),  // Snare noise mix
                        vdupq_n_f32(p[PARAM_SNARE_BODY] * 0.01f));  // Snare body resonance

    // Metal engine: param1 = inharmonicity (0-1), param2 = brightness (0-1)
    metal_engine_update(&synth->metal,
                        vdupq_n_f32(p[PARAM_METAL_INHARM] * 0.01f),   // Metal inharmonicity
                        vdupq_n_f32(p[PARAM_METAL_BRIGHT] * 0.01f));  // Metal brightness

    // Perc engine: param1 = ratio center (0-1), param2 = variation (0-1)
    perc_engine_update(&synth->perc,
                        vdupq_n_f32(p[PARAM_PERC_RATIO] * 0.01f),  // Perc ratio center
                        vdupq_n_f32(p[PARAM_PERC_VAR] * 0.01f));   // Perc variation

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
    int8_t depth1 = p[PARAM_LFO1_DEPTH];
    int8_t depth2 = p[PARAM_LFO2_DEPTH];

    lfo_smoother_set_rate(&synth->lfo_smooth, 0, p[PARAM_LFO1_RATE] * 0.01f, all_voices);
    lfo_smoother_set_rate(&synth->lfo_smooth, 1, p[PARAM_LFO2_RATE] * 0.01f, all_voices);
    lfo_smoother_set_depth(&synth->lfo_smooth, 0, depth1 * 0.01f, all_voices);
    lfo_smoother_set_depth(&synth->lfo_smooth, 1, depth2 * 0.01f, all_voices);
    lfo_smoother_set_target(&synth->lfo_smooth, 0, p[PARAM_LFO1_TARGET], all_voices);
    lfo_smoother_set_target(&synth->lfo_smooth, 1, p[PARAM_LFO2_TARGET], all_voices);

    // =================================================================
    // Update envelope shape and metal character (param 20)
    // EnvShape encoding: bit 7 = metal character (0=Cymbal, 1=Gong)
    //                    bits[6:0] = envelope ROM index (0-127)
    // =================================================================
    synth->current_env_shape = (uint8_t)p[PARAM_ENV_SHAPE];
    metal_engine_set_character(&synth->metal,
                               (uint32_t)(synth->current_env_shape >> 7));

    // =================================================================
    // Update Euclidean tuning offsets (param 16 / EuclTun)
    // Loads the per-voice semitone offset vector from the static lookup
    // table.  Applied at note-on so each voice plays a different pitch
    // derived from E(4,n): position[i] = floor(i * n / 4).
    // =================================================================
    {
        uint8_t mode = (uint8_t)p[PARAM_LFO2_SHAPE];
        if (mode >= EUCLID_MODE_COUNT) mode = 0;
        synth->euclid_offsets = vld1q_f32(EUCLID_OFFSETS[mode]);
    }
}

/**
 * standalone function similar to load_preset
 */
fast_inline void load_fm_preset(uint8_t idx, int8_t * params) {
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
    params[PARAM_LFO1_DEPTH] = p->lfo1_depth;  // -100..100, stored directly in int8_t

    // Page 5 (LFO2)
    params[PARAM_LFO2_SHAPE] = p->lfo2_shape;
    params[PARAM_LFO2_RATE] = p->lfo2_rate;
    params[PARAM_LFO2_TARGET] = p->lfo2_target;
    params[PARAM_LFO2_DEPTH] = p->lfo2_depth;  // -100..100, stored directly in int8_t

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

    // Initialize per-engine band filter states
    synth->kick_out_lpf.z1 = vdupq_n_f32(0.0f);
    synth->snare_hpf.z1    = vdupq_n_f32(0.0f);
    synth->snare_out_lpf.z1= vdupq_n_f32(0.0f);
    synth->metal_hpf.z1    = vdupq_n_f32(0.0f);
    synth->perc_hpf.z1     = vdupq_n_f32(0.0f);
    synth->perc_out_lpf.z1 = vdupq_n_f32(0.0f);

    // Initialize parameters
    synth->voice_active = vdupq_n_f32(0.0f);
    synth->voice_triggered = vdupq_n_u32(0);
    synth->voice_velocity = vdupq_n_f32(1.0f);
    synth->euclid_offsets = vdupq_n_f32(0.0f);  // Off: all voices unison
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
 * MIDI Note On handler with per-voice probability and note routing.
 *
 * Routing: each voice position has a dedicated drum note (kick=36, snare=38,
 * metal=42, perc=45 by default in midi_handler).  A note matching one of those
 * four only triggers voices at the matching position; any other note triggers
 * all voices (general trigger / sequencer mode).
 *
 * LFO phase sync: phases are reset on every trigger so a one-shot ramp at
 * slow rate acts as a secondary envelope.
 */
fast_inline void fm_perc_synth_note_on(fm_perc_synth_t* synth,
                                       uint8_t note,
                                       uint8_t velocity) {
    // ---------------------------------------------------------------
    // 1. MIDI note routing: build a position mask for this note.
    //    Positions 0-3 map to kick/snare/metal/perc notes respectively.
    //    Any non-drum note triggers all four positions.
    // ---------------------------------------------------------------
    uint32_t route_bits = 0xF;  // default: all voices eligible
    if (note == synth->midi.kick_note)  route_bits = 0x1;
    else if (note == synth->midi.snare_note) route_bits = 0x2;
    else if (note == synth->midi.metal_note) route_bits = 0x4;
    else if (note == synth->midi.perc_note)  route_bits = 0x8;

    const uint32_t route_lanes[4] = {
        (route_bits & 1) ? 0xFFFFFFFFU : 0U,
        (route_bits & 2) ? 0xFFFFFFFFU : 0U,
        (route_bits & 4) ? 0xFFFFFFFFU : 0U,
        (route_bits & 8) ? 0xFFFFFFFFU : 0U,
    };
    uint32x4_t route_mask = vld1q_u32(route_lanes);

    // ---------------------------------------------------------------
    // 2. Probability gate, masked by routing
    // ---------------------------------------------------------------
    uint32x4_t gate = probability_gate_neon(&synth->prng,
                                            synth->voice_probs[PARAM_VOICE1_PROB],
                                            synth->voice_probs[PARAM_VOICE2_PROB],
                                            synth->voice_probs[PARAM_VOICE3_PROB],
                                            synth->voice_probs[PARAM_VOICE4_PROB]);
    gate = vandq_u32(gate, route_mask);

    uint32_t gate_bits = 0;
    gate_bits |= (vgetq_lane_u32(gate, 0) ? 1 : 0) << 0;
    gate_bits |= (vgetq_lane_u32(gate, 1) ? 1 : 0) << 1;
    gate_bits |= (vgetq_lane_u32(gate, 2) ? 1 : 0) << 2;
    gate_bits |= (vgetq_lane_u32(gate, 3) ? 1 : 0) << 3;

    if (gate_bits == 0) return;

    // ---------------------------------------------------------------
    // 3. Per-voice velocity
    // ---------------------------------------------------------------
    float vel_scale = velocity / 127.0f;
    synth->voice_velocity = vbslq_f32(gate,
                                       vdupq_n_f32(vel_scale),
                                       synth->voice_velocity);

    // ---------------------------------------------------------------
    // 4. Set engine note for each triggered voice.
    //    Apply Euclidean tuning: voice i gets note + euclid_offsets[i]
    //    so the 4 voices are spread across a Euclidean pitch distribution.
    //    When EuclTun=Off, euclid_offsets = [0,0,0,0] (no change).
    // ---------------------------------------------------------------
    float32x4_t midi_note_v = vaddq_f32(vdupq_n_f32((float)note),
                                         synth->euclid_offsets);

    for (int v = 0; v < VOICE_ALLOC_MAX; v++) {
        if (gate_bits & (1 << v)) {
            uint32x4_t voice_mask = vceqq_u32(vdupq_n_u32(v),
                                               vld1q_u32((const uint32_t[]){0,1,2,3}));
            switch (synth->voice_engine[v]) {
                case ENGINE_KICK:
                    kick_engine_set_note(&synth->kick, voice_mask, midi_note_v);    break;
                case ENGINE_SNARE:
                    snare_engine_set_note(&synth->snare, voice_mask, midi_note_v);  break;
                case ENGINE_METAL:
                    metal_engine_set_note(&synth->metal, voice_mask, midi_note_v);  break;
                case ENGINE_PERC:
                    perc_engine_set_note(&synth->perc, voice_mask, midi_note_v);    break;
                case ENGINE_RESONANT:
                    resonant_synth_set_f0(&synth->resonant, voice_mask, midi_note_v); break;
            }
        }
    }

    // ---------------------------------------------------------------
    // 5. Track active notes for per-voice release
    // ---------------------------------------------------------------
    synth->midi.active_notes[note] |= (uint8_t)gate_bits;

    // ---------------------------------------------------------------
    // 6. LFO phase sync: reset both LFO phases on every trigger.
    //    A slow ramp (0.5 Hz) now acts as a one-shot attack/decay shape.
    // ---------------------------------------------------------------
    synth->lfo.phase1 = vdupq_n_f32(0.0f);
    synth->lfo.phase2 = vdupq_n_f32(LFO_PHASE_OFFSET);

    // ---------------------------------------------------------------
    // 7. Trigger envelope for gated voices
    // ---------------------------------------------------------------
    synth->voice_triggered = gate;
    // Mask to lower 7 bits: bit 7 encodes metal character, not envelope index
    neon_envelope_trigger(&synth->envelope, gate,
                          synth->current_env_shape & 0x7Fu);
}

/**
 * MIDI Note Off — triggers per-voice envelope release for voices that
 * were triggered by this specific note.
 */
fast_inline void fm_perc_synth_note_off(fm_perc_synth_t* synth, uint8_t note) {
    uint8_t releasing[4];
    uint32_t num_releasing = midi_note_off(&synth->midi, note, releasing);

    if (num_releasing == 0) return;

    // Build NEON voice mask for releasing voices
    uint32_t rel_bits = 0;
    for (uint32_t i = 0; i < num_releasing; i++)
        rel_bits |= (1u << releasing[i]);

    const uint32_t rel_lanes[4] = {
        (rel_bits & 1) ? 0xFFFFFFFFU : 0U,
        (rel_bits & 2) ? 0xFFFFFFFFU : 0U,
        (rel_bits & 4) ? 0xFFFFFFFFU : 0U,
        (rel_bits & 8) ? 0xFFFFFFFFU : 0U,
    };
    neon_envelope_release(&synth->envelope, vld1q_u32(rel_lanes));
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
                        vgetq_lane_f32(synth->lfo_smooth.current_depth1,  0) * 100.0f,
                        vgetq_lane_f32(synth->lfo_smooth.current_depth2,  0) * 100.0f,
                        vgetq_lane_f32(synth->lfo_smooth.current_rate1,   0) * 100.0f,
                        vgetq_lane_f32(synth->lfo_smooth.current_rate2,   0) * 100.0f);

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
    // LFO target guide — all targets wired; character notes:
    //   PITCH(1)    Purely percussive: pitch sweep / flam tuning at slow rate,
    //               vibrato at medium rate, FM-pitch crunch at near-audio rate.
    //   INDEX(2)    Percussive: brightness / spectral density modulation.
    //               High depth causes AM beating when both LFOs target INDEX.
    //   ENV(3)      Can shift toward synth/melodic: acts as amplitude envelope
    //               re-shaper — slow rates create dynamic variation per hit,
    //               fast rates create tremolo, very fast = AM synth character.
    //   LFO2_PHASE(4)/LFO1_PHASE(5)  LFO cross-modulation (frequency FM between
    //               the two LFOs). Keeps percussive but adds movement to sweep.
    //   RES_FREQ(6)/RESONANCE(7)/RES_MORPH(9)  Only affect the resonant engine;
    //               silent when voice allocation has no resonant engine active.
    //   NOISE_MIX(8) Modulates snare noise/tone balance AND metal FM index depth.
    //               Both wired: snare via additive offset in process(), metal via
    //               brightness_add parameter.
    //   METAL_GATE(10) Per-sample amplitude gate on metal engine only. Use with
    //               Ramp shape + positive depth for open/closed hi-hat effect.
    //               LFO phase resets on trigger, so Ramp acts as one-shot gate.
    float32x4_t index_add      = vdupq_n_f32(0.0f);
    float32x4_t lfo_pitch_mult = vdupq_n_f32(1.0f);

    // =================================================================
    // Apply LFO modulations to pitch (target 1)
    // =================================================================
    if (synth->lfo.target1 == LFO_TARGET_PITCH || synth->lfo.target2 == LFO_TARGET_PITCH) {
        // Initialize to zero – both LFOs may contribute independently
        float32x4_t pitch_octaves = vdupq_n_f32(0.0f);
        // +/- 2 octaves maximum depth per LFO; sum both if both target pitch
        if (synth->lfo.target1 == LFO_TARGET_PITCH)
            pitch_octaves = vaddq_f32(pitch_octaves,
                                      vmulq_f32(lfo1, vmulq_n_f32(synth->lfo.depth1, 2.0f)));
        if (synth->lfo.target2 == LFO_TARGET_PITCH)
            pitch_octaves = vaddq_f32(pitch_octaves,
                                      vmulq_f32(lfo2, vmulq_n_f32(synth->lfo.depth2, 2.0f)));
        // Exponential pitch multiplier passed to each engine's process(); do NOT
        // write back into carrier_freq_base – that would permanently drift the tuning.
        lfo_pitch_mult = exp2_neon(pitch_octaves);
    }

    if (synth->lfo.target1 == LFO_TARGET_INDEX || synth->lfo.target2 == LFO_TARGET_INDEX) {
        float32x4_t mod = (synth->lfo.target1 == LFO_TARGET_INDEX) ? lfo1 : lfo2;
        float32x4_t depth = (synth->lfo.target1 == LFO_TARGET_INDEX) ? synth->lfo.depth1 : synth->lfo.depth2;
        index_add = vaddq_f32(index_add, vmulq_f32(mod, depth));
        // Clamp so engines never receive an index outside [-1, 1]
        index_add = vmaxq_f32(vminq_f32(index_add, vdupq_n_f32(1.0f)), vdupq_n_f32(-1.0f));

        // Inject index modulation — each engine_update2 clamps internally to [0,1]
    }

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

    float32x4_t noise_add = vdupq_n_f32(0.0f);
    // =================================================================
    // LFO → NOISE_MIX (target 8): modulate snare noise/tone blend and metal brightness
    // =================================================================
    if (synth->lfo.target1 == LFO_TARGET_NOISE_MIX ||
        synth->lfo.target2 == LFO_TARGET_NOISE_MIX) {
        float32x4_t mod   = (synth->lfo.target1 == LFO_TARGET_NOISE_MIX) ? lfo1 : lfo2;
        float32x4_t depth = (synth->lfo.target1 == LFO_TARGET_NOISE_MIX)
                            ? synth->lfo.depth1 : synth->lfo.depth2;
        // Bipolar LFO (-1..1) * depth as additive offset to noise_mix/brightness
        noise_add = vmulq_f32(
            vsubq_f32(vmulq_f32(mod, vdupq_n_f32(2.0f)), vdupq_n_f32(1.0f)),
            depth);
        // noise_add is applied in:
        //   snare_engine_process (line ~213): noise_mix_mod = noise_mix + noise_add
        //   metal_engine_process (brightness_add param): base_index += brightness_add
        // NOTE: snare_engine_update_noise() is intentionally NOT called here —
        //       it writes to snare->noise_mix which is used as a base level,
        //       and calling it per-sample would cause exploding feedback.
    }

    // =================================================================
    // LFO → RES_MORPH (target 9): sweep resonant filter morph each sample
    // =================================================================
    if (synth->lfo.target1 == LFO_TARGET_RES_MORPH ||
        synth->lfo.target2 == LFO_TARGET_RES_MORPH) {
        float32x4_t mod   = (synth->lfo.target1 == LFO_TARGET_RES_MORPH) ? lfo1 : lfo2;
        float32x4_t depth = (synth->lfo.target1 == LFO_TARGET_RES_MORPH)
                            ? synth->lfo.depth1 : synth->lfo.depth2;
        // Bipolar offset on top of the static resonant_morph param
        float lfo_bipolar = vgetq_lane_f32(
            vsubq_f32(vmulq_f32(mod, vdupq_n_f32(2.0f)), vdupq_n_f32(1.0f)), 0);
        float modulated_morph = synth->resonant_morph
                                + lfo_bipolar * vgetq_lane_f32(depth, 0);
        // Clamp to valid range
        if (modulated_morph < 0.0f) modulated_morph = 0.0f;
        if (modulated_morph > 1.0f) modulated_morph = 1.0f;
        apply_resonant_morph(&synth->resonant, modulated_morph,
                             synth->params[PARAM_RES_MODE]);
    }

    // =================================================================
    // LFO → METAL_GATE (target 10): open/closed hi-hat amplitude gate
    // Inspired by the TR-909: same oscillator bank, two different VCA
    // decay times. Here the LFO provides the variable-length gate.
    //
    // Usage: set L1Shape=Ramp, L1Target=MetalGate, L1Depth=+50..+100%.
    //   L1Rate high → gate closes fast → closed hi-hat character.
    //   L1Rate low  → gate closes slowly → open hi-hat character.
    //   L1Depth 0%  → metal_gate = 1.0 always (gate disabled, fully open).
    //   L1Depth negative → no effect (same as depth 0%).
    //
    // Because LFO phase resets on every trigger (fm_perc_synth_note_on),
    // the Ramp starts from 0.0 on each hit, acting as a one-shot decay gate.
    // =================================================================
    float32x4_t metal_gate = vdupq_n_f32(1.0f);  // Default: fully open (no gating)
    if (synth->lfo.target1 == LFO_TARGET_METAL_GATE ||
        synth->lfo.target2 == LFO_TARGET_METAL_GATE) {
        float32x4_t mod   = (synth->lfo.target1 == LFO_TARGET_METAL_GATE) ? lfo1 : lfo2;
        float32x4_t depth = (synth->lfo.target1 == LFO_TARGET_METAL_GATE)
                            ? synth->lfo.depth1 : synth->lfo.depth2;
        // gate = clamp(1.0 - lfo * depth, 0, 1)
        // Ramp LFO: 0 at trigger → 1 at end of period.
        // Positive depth: 1.0 at trigger → 0.0 as lfo reaches 1/depth.
        // Negative depth: no effect (gate stays ≥1.0, clamped to 1.0).
        float32x4_t raw = vsubq_f32(vdupq_n_f32(1.0f), vmulq_f32(mod, depth));
        metal_gate = vmaxq_f32(raw, vdupq_n_f32(0.0f));
        metal_gate = vminq_f32(metal_gate, vdupq_n_f32(1.0f));
    }

    // =================================================================
    // Process each engine with its voice mask
    // =================================================================
    float32x4_t kick_out = kick_engine_process(&synth->kick, env,
                                               synth->engine_mask[ENGINE_KICK],
                                               lfo_pitch_mult, index_add);
    float32x4_t snare_out = snare_engine_process(&synth->snare, env,
                                                 synth->engine_mask[ENGINE_SNARE],
                                                 lfo_pitch_mult, index_add, noise_add);
    float32x4_t metal_out = metal_engine_process(&synth->metal, env,
                                                 synth->engine_mask[ENGINE_METAL],
                                                 lfo_pitch_mult, index_add, noise_add,
                                                 metal_gate);
    float32x4_t perc_out = perc_engine_process(&synth->perc, env,
                                               synth->engine_mask[ENGINE_PERC],
                                               lfo_pitch_mult, index_add);
    float32x4_t resonant_out = resonant_synth_process(&synth->resonant, env,
                                                       synth->engine_mask[ENGINE_RESONANT]);

    // =================================================================
    // Per-engine band filters — spectral band separation.
    // Each engine is constrained to its natural frequency range so voices
    // don't mask each other in the mix (kick rumble vs snare crack, etc.).
    // HPF is implemented as: output = input - LPF(input, cutoff).
    // All alphas are precomputed constants (no division per sample).
    // =================================================================

    // Kick: LP 250 Hz — preserves body, removes high-frequency clash
    kick_out = one_pole_lpf_a(&synth->kick_out_lpf, kick_out, FILT_KICK_LP_A);

    // Snare: HP 100 Hz (remove kick rumble) then LP 7000 Hz (remove hash)
    {
        float32x4_t snare_lp = one_pole_lpf_a(&synth->snare_hpf, snare_out, FILT_SNARE_HP_A);
        snare_out = vsubq_f32(snare_out, snare_lp);           // HP at 100 Hz
        snare_out = one_pole_lpf_a(&synth->snare_out_lpf, snare_out, FILT_SNARE_LP_A);
    }

    // Metal/cymbal: HP 800 Hz — removes lows, keeps shimmer
    {
        float32x4_t metal_lp = one_pole_lpf_a(&synth->metal_hpf, metal_out, FILT_METAL_HP_A);
        metal_out = vsubq_f32(metal_out, metal_lp);           // HP at 800 Hz
    }

    // Perc/tom: HP 80 Hz (remove sub rumble) then LP 3000 Hz (remove clash with metal)
    {
        float32x4_t perc_lp = one_pole_lpf_a(&synth->perc_hpf, perc_out, FILT_PERC_HP_A);
        perc_out = vsubq_f32(perc_out, perc_lp);              // HP at 80 Hz
        perc_out = one_pole_lpf_a(&synth->perc_out_lpf, perc_out, FILT_PERC_LP_A);
    }

    // Resonant engine: full-range pass — it is already a filter

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
    // Kick ducking: when kick fires, reduce non-kick voice gain by up to
    // KICK_DUCK_AMOUNT so the kick punches through without muddiness.
    // Zero effect when kick is not in the current voice allocation.
    // Future: expose KICK_DUCK_AMOUNT as a parameter (see PROGRESS.md).
    // =================================================================
    static const float KICK_DUCK_AMOUNT = 0.30f;
    {
        // Select kick lane envelope levels; 0 everywhere else
        float32x4_t kick_env = vbslq_f32(synth->engine_mask[ENGINE_KICK],
                                          synth->envelope.level,
                                          vdupq_n_f32(0.0f));
        float kick_level = neon_horizontal_sum(kick_env);  // ≤1 voice active → scalar
        float duck_gain  = 1.0f - KICK_DUCK_AMOUNT * kick_level;
        // Apply duck_gain to non-kick lanes, 1.0 to the kick lane itself
        float32x4_t duck_vec = vbslq_f32(vmvnq_u32(synth->engine_mask[ENGINE_KICK]),
                                          vdupq_n_f32(duck_gain),
                                          vdupq_n_f32(1.0f));
        mixed = vmulq_f32(mixed, duck_vec);
    }

    // =================================================================
    // Horizontal sum using NEON vpadd
    // =================================================================
    float sum = neon_horizontal_sum(mixed);

    // Apply master gain
    return sum * synth->master_gain;
}
