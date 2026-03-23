#pragma once

/**
 * @file fm_voices.h
 * @brief NEON-aligned data structures for 4-voice FM synthesis
 *
 * FIXED: Using central constants from constants.h
 */

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"
#include "envelope_rom.h"
#include "lfo_enhanced.h"

    // Maximum operators per voice
    constexpr int MAX_OPERATORS = 4;

/**
 * Operator data for 4 voices (SoA format)
 */
typedef struct {
    float32x4_t phase[MAX_OPERATORS];
    float32x4_t freq[MAX_OPERATORS];
    float32x4_t index[MAX_OPERATORS];
    float32x4_t output[MAX_OPERATORS];
} fm_operators_t;

/**
 * Per-voice data
 */
typedef struct {
    uint32x4_t active;
    float32x4_t envelope;
    uint32x4_t env_stage;
    uint32x4_t stage_time;
    float32x4_t param1;
    float32x4_t param2;
    float32x4_t gain;

    // Note frequency (can use interval ratios from constants.h)
    float32x4_t note_freq;
} fm_voices_t;

/**
 * Complete FM synthesis state
 */
typedef struct {
    fm_operators_t ops __attribute__((aligned(VECTOR_ALIGN)));
    fm_voices_t voices __attribute__((aligned(VECTOR_ALIGN)));
    float32x4_t lfo_phase[2] __attribute__((aligned(VECTOR_ALIGN)));
    uint32x4_t prng_state[4] __attribute__((aligned(VECTOR_ALIGN)));
    uint8_t params[24];
} fm_state_t;

// Ensure proper alignment
static_assert(sizeof(fm_state_t) % VECTOR_ALIGN == 0,
              "fm_state_t must be 16-byte aligned for NEON");

/**
 * Convert MIDI note to frequency using interval ratios
 */
fast_inline float32x4_t midi_to_freq_neon(uint32x4_t midi_notes) {
    // This would use the interval ratios from constants.h
    // to build frequencies efficiently
    float32x4_t a4_freq = vdupq_n_f32(A4_FREQ);
    float32x4_t a4_midi = vdupq_n_f32(A4_MIDI);

    // Calculate semitone offset (TODO: use for full pitch calc; placeholder for now)
    float32x4_t semitone_offset = vsubq_f32(vcvtq_f32_u32(midi_notes), a4_midi);
    (void)semitone_offset;

    // 2^(offset/12) using approximation or table
    // In practice, you'd use a fast approximation

    return a4_freq; // Placeholder
}

/**
 * Initialize FM state
 */
fast_inline void fm_state_init(fm_state_t* state) {
    for (int op = 0; op < MAX_OPERATORS; op++) {
        state->ops.phase[op] = vdupq_n_f32(0.0f);
        state->ops.freq[op] = vdupq_n_f32(0.0f);
        state->ops.index[op] = vdupq_n_f32(0.0f);
        state->ops.output[op] = vdupq_n_f32(0.0f);
    }

    state->voices.active = vdupq_n_u32(0);
    state->voices.envelope = vdupq_n_f32(0.0f);
    state->voices.env_stage = vdupq_n_u32(ENV_STATE_OFF);  // Using constant
    state->voices.stage_time = vdupq_n_u32(0);
    state->voices.param1 = vdupq_n_f32(0.5f);
    state->voices.param2 = vdupq_n_f32(0.5f);
    state->voices.gain = vdupq_n_f32(0.25f);
    state->voices.note_freq = vdupq_n_f32(A4_FREQ);

    state->lfo_phase[0] = vdupq_n_f32(0.0f);
    state->lfo_phase[1] = vdupq_n_f32(LFO_PHASE_OFFSET);  // Using constant

    for (int i = 0; i < 4; i++) {
        state->prng_state[i] = vdupq_n_u32(0x9E3779B9);
    }
}