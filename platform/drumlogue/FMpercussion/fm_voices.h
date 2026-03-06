/**
 *  @file fm_voices.h
 *  @brief NEON-aligned data structures for 4-voice FM synthesis
 *
 *  Structure of Arrays (SoA) layout for maximum SIMD efficiency
 */

#pragma once

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"
#include "envelope_rom.h"

// Maximum operators per voice
#define MAX_OPERATORS 4

/**
 * Operator data for 4 voices (SoA format)
 * Each vector contains data for one operator across all 4 voices
 */
typedef struct {
    // Phase accumulators (0 to 2π)
    float32x4_t phase[MAX_OPERATORS];

    // Frequency increments (radians per sample)
    float32x4_t freq[MAX_OPERATORS];

    // Modulation indices (0-1 scale, multiplied by envelope)
    float32x4_t index[MAX_OPERATORS];

    // Current output values
    float32x4_t output[MAX_OPERATORS];
} fm_operators_t;

/**
 * Per-voice data (still vectorized across 4 voices)
 */
typedef struct {
    // Voice enable/trigger states
    uint32x4_t active;

    // Current envelope level (0-1)
    float32x4_t envelope;

    // Envelope stage (0=attack, 1=decay, 2=release, 3=off)
    uint32x4_t env_stage;

    // Time in current stage (samples)
    uint32x4_t stage_time;

    // Voice-specific parameters (from pages 2-3)
    float32x4_t param1;  // KickParam1, SnareParam1, MetalParam1, PercParam1
    float32x4_t param2;  // KickParam2, SnareParam2, MetalParam2, PercParam2

    // Voice gain (for mixing)
    float32x4_t gain;
} fm_voices_t;

/**
 * Complete FM synthesis state
 * 16-byte aligned for NEON loads/stores
 */
typedef struct {
    // Operator data (4 operators × 4 voices)
    fm_operators_t ops __attribute__((aligned(16)));

    // Voice data
    fm_voices_t voices __attribute__((aligned(16)));

    // LFO states (2 LFOs × 4 voices)
    float32x4_t lfo_phase[2] __attribute__((aligned(16)));

    // PRNG for probability gate
    uint32x4_t prng_state[4] __attribute__((aligned(16)));

    // Current parameter values (cached for smoothing)
    uint8_t params[24];
} fm_state_t;

// 128 predefined ADR envelopes
typedef struct {
    uint8_t attack_ms;   // 0-100 ms
    uint8_t decay_ms;    // 0-500 ms
    uint8_t release_ms;  // 0-500 ms
    uint8_t curve;       // 0=linear, 1=exp, etc.
} env_shape_t;

// Root-3rd-5th: 0, 4, 7 semitones
const float CHORD_INTERVALS[3] = {
    1.0f,                    // Root (unison)
    powf(2.0f, 4.0f/12.0f),  // Major 3rd: 2^(4/12)
    powf(2.0f, 7.0f/12.0f)   // Perfect 5th: 2^(7/12)
};

float chord_lfo(float phase) {
    // phase 0-1 maps to 0,1,2 steps
    int step = (int)(phase * 3);
    return CHORD_INTERVALS[step];
}

// Ensure structures are properly aligned
static_assert(sizeof(fm_state_t) % 16 == 0,
              "fm_state_t must be 16-byte aligned for NEON");

/**
 * Initialize FM state
 */
fast_inline void fm_state_init(fm_state_t* state) {
    // Zero all vectors
    for (int op = 0; op < MAX_OPERATORS; op++) {
        state->ops.phase[op] = vdupq_n_f32(0.0f);
        state->ops.freq[op] = vdupq_n_f32(0.0f);
        state->ops.index[op] = vdupq_n_f32(0.0f);
        state->ops.output[op] = vdupq_n_f32(0.0f);
    }

    state->voices.active = vdupq_n_u32(0);
    state->voices.envelope = vdupq_n_f32(0.0f);
    state->voices.env_stage = vdupq_n_u32(3);  // 3 = off
    state->voices.stage_time = vdupq_n_u32(0);
    state->voices.param1 = vdupq_n_f32(0.5f);
    state->voices.param2 = vdupq_n_f32(0.5f);
    state->voices.gain = vdupq_n_f32(0.25f);  // -12dB each

    state->lfo_phase[0] = vdupq_n_f32(0.0f);
    state->lfo_phase[1] = vdupq_n_f32(0.0f);

    // Initialize PRNG (will be seeded properly later)
    for (int i = 0; i < 4; i++) {
        state->prng_state[i] = vdupq_n_u32(0x9E3779B9);
    }
}

// ========== UNIT TEST ==========
#ifdef TEST_STRUCTURES

#include <stdio.h>

void test_alignment() {
    fm_state_t state;

    printf("State address: %p\n", &state);
    printf("Alignment: %lu\n", (uintptr_t)&state % 16);
    assert(((uintptr_t)&state % 16) == 0);

    printf("Size of fm_state_t: %lu bytes\n", sizeof(fm_state_t));
    printf("Size should be multiple of 16: %s\n",
           sizeof(fm_state_t) % 16 == 0 ? "PASS" : "FAIL");

    printf("Structure alignment test PASSED\n");
}

int main() {
    test_alignment();
    return 0;
}

#endif // TEST_STRUCTURES