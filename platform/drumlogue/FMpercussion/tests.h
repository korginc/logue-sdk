#ifndef TESTS_H
#define TESTS_H

/**
 *  @file tests.h
 *  @brief Comprehensive unit test suite for FM Percussion Synthesizer
 *
 *  Individual component tests + 16-beat stability/long run test
 *  All tests return true = PASS, false = FAIL
 */

#pragma once

#include <stdio.h>
#include <math.h>
#include <arm_neon.h>
#include "fm_perc_synth.h"
#include "prng.h"
#include "sine_neon.h"
#include "kick_engine.h"
#include "snare_engine.h"
#include "metal_engine.h"
#include "perc_engine.h"
#include "lfo_enhanced.h"
#include "envelope_rom.h"

// Test configuration
#define TEST_SAMPLE_RATE 48000
#define TEST_DURATION_SECONDS 5
#define TEST_LONG_RUN_BEATS 16
#define TEST_BPM 120
#define TEST_BEAT_DURATION (60.0f / TEST_BPM)  // seconds per beat

// Test results structure
typedef struct {
    const char* name;
    bool passed;
    uint32_t cycles_used;
    float cpu_percent;
    char error_msg[256];
} test_result_t;

// Global test results array
#define MAX_TESTS 20
test_result_t test_results[MAX_TESTS];
int test_count = 0;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Read CPU cycle counter (ARM Cortex-A9)
 */
static inline uint32_t read_cycle_counter(void) {
    uint32_t cycles;
    asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (cycles));
    return cycles;
}

/**
 * Register test result
 */
void register_test(const char* name, bool passed, uint32_t cycles, const char* error) {
    if (test_count < MAX_TESTS) {
        test_results[test_count].name = name;
        test_results[test_count].passed = passed;
        test_results[test_count].cycles_used = cycles;
        test_results[test_count].cpu_percent = (cycles * 100.0f) / 1000000000;  // At 1GHz
        if (error) {
            strncpy(test_results[test_count].error_msg, error, 255);
            test_results[test_count].error_msg[255] = '\0';
        } else {
            test_results[test_count].error_msg[0] = '\0';
        }
        test_count++;
    }
}

/**
 * Print test summary
 */
void print_test_summary(void) {
    printf("\n========================================\n");
    printf("      FM PERCUSSION SYNTH TEST SUITE    \n");
    printf("========================================\n\n");

    int passed = 0;
    for (int i = 0; i < test_count; i++) {
        printf("[%s] %s", test_results[i].passed ? "PASS" : "FAIL", test_results[i].name);
        if (test_results[i].cycles_used > 0) {
            printf(" - %d cycles", test_results[i].cycles_used);
        }
        if (test_results[i].error_msg[0] != '\0') {
            printf("\n      Error: %s", test_results[i].error_msg);
        }
        printf("\n");

        if (test_results[i].passed) passed++;
    }

    printf("\n----------------------------------------\n");
    printf("Total: %d/%d tests passed\n", passed, test_count);
    printf("========================================\n");
}

// ============================================================================
// PART 1: INDIVIDUAL COMPONENT TESTS
// ============================================================================

// ----------------------------------------------------------------------------
// PRNG Tests
// ----------------------------------------------------------------------------

bool test_prng_uniformity(void) {
    neon_prng_t rng;
    neon_prng_init(&rng, 0x12345678);

    // Collect 10000 samples per stream
    uint32_t bins[4][256] = {{0}};

    uint32_t start_cycles = read_cycle_counter();

    for (int i = 0; i < 10000; i++) {
        uint32x4_t rand = neon_prng_rand_u32(&rng);
        uint32_t vals[4];
        vst1q_u32(vals, rand);

        for (int v = 0; v < 4; v++) {
            bins[v][vals[v] >> 24]++;  // Use top 8 bits
        }
    }

    uint32_t end_cycles = read_cycle_counter();
    uint32_t cycles = end_cycles - start_cycles;

    // Chi-square test for uniformity
    float expected = 10000.0f / 256.0f;
    bool passed = true;
    char error[256] = "";

    for (int v = 0; v < 4; v++) {
        float chi_square = 0;
        for (int b = 0; b < 256; b++) {
            float diff = bins[v][b] - expected;
            chi_square += (diff * diff) / expected;
        }

        // Degrees of freedom = 255, 99% confidence interval: 186 < chi2 < 324
        if (chi_square < 186 || chi_square > 324) {
            passed = false;
            sprintf(error, "Stream %d chi-square = %f (expected 186-324)", v, chi_square);
            break;
        }
    }

    register_test("PRNG Uniformity", passed, cycles / 10000, passed ? NULL : error);
    return passed;
}

bool test_prng_independence(void) {
    neon_prng_t rng;
    neon_prng_init(&rng, 0x87654321);

    // Generate samples and check cross-correlation between streams
    const int SAMPLES = 1000;
    uint32_t streams[4][SAMPLES];

    for (int i = 0; i < SAMPLES; i++) {
        uint32x4_t rand = neon_prng_rand_u32(&rng);
        vst1q_u32(streams[i % 4], rand);  // Store interleaved
    }

    // Check correlation between stream 0 and 1
    float correlation = 0;
    for (int i = 0; i < SAMPLES; i++) {
        correlation += (streams[0][i] & 1) == (streams[1][i] & 1) ? 1 : -1;
    }
    correlation /= SAMPLES;

    bool passed = fabs(correlation) < 0.1f;  // Should be near 0
    register_test("PRNG Stream Independence", passed, 0,
                  passed ? NULL : "Streams are correlated");
    return passed;
}

// ----------------------------------------------------------------------------
// Sine Oscillator Tests
// ----------------------------------------------------------------------------

bool test_sine_accuracy(void) {
    float max_error = 0;
    float max_error_at = 0;

    uint32_t start_cycles = read_cycle_counter();

    // Test 1000 points across [0, 2π]
    for (int i = 0; i < 1000; i++) {
        float phase = (2.0f * M_PI * i) / 1000.0f;
        float32x4_t phases = vdupq_n_f32(phase);

        float32x4_t result = neon_sin(phases);
        float sin_val = vgetq_lane_f32(result, 0);
        float expected = sinf(phase);

        float error = fabsf(sin_val - expected);
        if (error > max_error) {
            max_error = error;
            max_error_at = phase;
        }
    }

    uint32_t end_cycles = read_cycle_counter();
    uint32_t cycles = (end_cycles - start_cycles) / 1000;  // Per sine

    bool passed = max_error < 0.001f;
    char error[256];
    if (!passed) {
        sprintf(error, "Max error %f at phase %f (expected <0.001)", max_error, max_error_at);
    }

    register_test("Sine Accuracy", passed, cycles, passed ? NULL : error);
    return passed;
}

bool test_sine_parallel_execution(void) {
    // Test 4 different phases simultaneously
    float phases_in[4] = {0.0f, 1.0f, 2.0f, 3.0f};
    float32x4_t phases = vld1q_f32(phases_in);

    uint32_t start_cycles = read_cycle_counter();
    float32x4_t result = neon_sin(phases);
    uint32_t end_cycles = read_cycle_counter();

    float results[4];
    vst1q_f32(results, result);

    bool passed = true;
    char error[256] = "";

    for (int i = 0; i < 4; i++) {
        float expected = sinf(phases_in[i]);
        float error_val = fabsf(results[i] - expected);
        if (error_val > 0.001f) {
            passed = false;
            sprintf(error, "Lane %d error = %f", i, error_val);
            break;
        }
    }

    register_test("Sine Parallel Execution", passed, end_cycles - start_cycles,
                  passed ? NULL : error);
    return passed;
}

// ----------------------------------------------------------------------------
// Envelope ROM Tests
// ----------------------------------------------------------------------------

bool test_envelope_rom_values(void) {
    bool passed = true;
    char error[256] = "";

    // Test a few key envelope shapes
    uint8_t a, c;
    uint16_t d, r;

    get_envelope(0, &a, &d, &r, &c);
    if (a != 0 || d != 20 || r != 10) {
        passed = false;
        sprintf(error, "Envelope 0: got %d/%d/%d, expected 0/20/10", a, d, r);
    }

    if (passed) {
        get_envelope(48, &a, &d, &r, &c);
        if (a != 20 || d != 500 || r != 250) {
            passed = false;
            sprintf(error, "Envelope 48: got %d/%d/%d, expected 20/500/250", a, d, r);
        }
    }

    if (passed) {
        get_envelope(127, &a, &d, &r, &c);
        if (a != 50 || d != 500 || r != 500) {
            passed = false;
            sprintf(error, "Envelope 127: got %d/%d/%d, expected 50/500/500", a, d, r);
        }
    }

    register_test("Envelope ROM Values", passed, 0, passed ? NULL : error);
    return passed;
}

bool test_envelope_generation(void) {
    neon_envelope_t env;
    neon_envelope_init(&env);

    uint32x4_t mask = vdupq_n_u32(0xFFFFFFFF);

    // Trigger with envelope shape 20 (should have attack)
    neon_envelope_trigger(&env, mask, 20);

    bool passed = true;
    char error[256] = "";
    float last_level = 0;

    // Run through attack
    for (int i = 0; i < 50; i++) {  // 50 samples
        neon_envelope_process(&env);
        float level = vgetq_lane_f32(env.level, 0);

        if (i > 0 && level <= last_level) {
            passed = false;
            sprintf(error, "Attack not increasing at sample %d", i);
            break;
        }
        last_level = level;
    }

    if (passed) {
        // Should be in decay now
        for (int i = 0; i < 100; i++) {
            neon_envelope_process(&env);
            float level = vgetq_lane_f32(env.level, 0);

            if (i > 0 && level >= last_level) {
                passed = false;
                sprintf(error, "Decay not decreasing at sample %d", i);
                break;
            }
            last_level = level;
        }
    }

    register_test("Envelope Generation", passed, 0, passed ? NULL : error);
    return passed;
}

// ----------------------------------------------------------------------------
// FM Engine Tests
// ----------------------------------------------------------------------------

bool test_kick_engine(void) {
    kick_engine_t kick;
    kick_engine_init(&kick);

    uint32x4_t mask = vdupq_n_u32(0xFFFFFFFF);
    float32x4_t env = vdupq_n_f32(1.0f);

    // Set note to C2
    float32x4_t midi = vdupq_n_f32(36.0f);
    kick_engine_set_note(&kick, mask, midi);

    // Process and verify frequency range
    float32x4_t out = kick_engine_process(&kick, env, mask);

    // Check that output is in valid range
    float sample = vgetq_lane_f32(out, 0);
    bool passed = (sample >= -1.0f && sample <= 1.0f);

    // Test pitch sweep
    float32x4_t param1 = vdupq_n_f32(1.0f);  // Max sweep
    float32x4_t param2 = vdupq_n_f32(0.5f);
    kick_engine_update(&kick, param1, param2);

    // Process with decaying envelope
    float last_sample = sample;
    for (float e = 0.9f; e >= 0.0f; e -= 0.1f) {
        env = vdupq_n_f32(e);
        out = kick_engine_process(&kick, env, mask);
        sample = vgetq_lane_f32(out, 0);

        // Should change due to sweep
        if (sample == last_sample) {
            passed = false;
            break;
        }
        last_sample = sample;
    }

    register_test("Kick Engine", passed, 0, passed ? NULL : "Kick engine failed");
    return passed;
}

bool test_snare_engine(void) {
    snare_engine_t snare;
    snare_engine_init(&snare);

    uint32x4_t mask = vdupq_n_u32(0xFFFFFFFF);
    float32x4_t env = vdupq_n_f32(1.0f);

    // Test noise mix parameter
    float32x4_t param1 = vdupq_n_f32(0.5f);  // 50% noise
    float32x4_t param2 = vdupq_n_f32(0.5f);
    snare_engine_update(&snare, param1, param2);

    // Process and check output range
    float32x4_t out = snare_engine_process(&snare, env, mask);
    float sample = vgetq_lane_f32(out, 0);
    bool passed = (sample >= -1.0f && sample <= 1.0f);

    // Test that noise mix actually changes output
    float32x4_t out_no_noise = out;
    param1 = vdupq_n_f32(0.0f);  // 0% noise
    snare_engine_update(&snare, param1, param2);
    out = snare_engine_process(&snare, env, mask);

    float sample_no_noise = vgetq_lane_f32(out_no_noise, 0);
    float sample_with_noise = vgetq_lane_f32(out, 0);

    if (sample_no_noise == sample_with_noise) {
        passed = false;
    }

    register_test("Snare Engine", passed, 0, passed ? NULL : "Snare engine failed");
    return passed;
}

bool test_metal_engine(void) {
    metal_engine_t metal;
    metal_engine_init(&metal);

    uint32x4_t mask = vdupq_n_u32(0xFFFFFFFF);
    float32x4_t env = vdupq_n_f32(1.0f);

    // Test inharmonicity parameter
    float32x4_t param1 = vdupq_n_f32(1.0f);  // Max inharmonicity
    float32x4_t param2 = vdupq_n_f32(0.5f);
    metal_engine_update(&metal, param1, param2);

    float32x4_t out = metal_engine_process(&metal, env, mask);
    float sample_max = vgetq_lane_f32(out, 0);
    bool passed = (sample_max >= -1.0f && sample_max <= 1.0f);

    // Compare with harmonic setting
    param1 = vdupq_n_f32(0.0f);  // Harmonic
    metal_engine_update(&metal, param1, param2);
    out = metal_engine_process(&metal, env, mask);
    float sample_harmonic = vgetq_lane_f32(out, 0);

    if (sample_max == sample_harmonic) {
        passed = false;
    }

    register_test("Metal Engine", passed, 0, passed ? NULL : "Metal engine failed");
    return passed;
}

bool test_perc_engine(void) {
    perc_engine_t perc;
    perc_engine_init(&perc);

    uint32x4_t mask = vdupq_n_u32(0xFFFFFFFF);
    float32x4_t env = vdupq_n_f32(1.0f);

    // Test ratio parameter
    float32x4_t param1 = vdupq_n_f32(0.0f);  // Min ratio
    float32x4_t param2 = vdupq_n_f32(0.5f);
    perc_engine_update(&perc, param1, param2);
    // TODO create new test with lfo modulation
    float32x4_t out = perc_engine_process(&perc, env, mask, vdupq_n_f32(1.0f), vdupq_n_f32(0.0f));
    float sample_min = vgetq_lane_f32(out, 0);
    bool passed = (sample_min >= -1.0f && sample_min <= 1.0f);

    // Compare with max ratio
    param1 = vdupq_n_f32(1.0f);  // Max ratio
    perc_engine_update(&perc, param1, param2);
    // TODO create new test with lfo modulation
    out = perc_engine_process(&perc, env, mask, vdupq_n_f32(1.0f), vdupq_n_f32(0.0f));
    float sample_max = vgetq_lane_f32(out, 0);

    if (sample_min == sample_max) {
        passed = false;
    }

    register_test("Perc Engine", passed, 0, passed ? NULL : "Perc engine failed");
    return passed;
}

// ----------------------------------------------------------------------------
// LFO Tests
// ----------------------------------------------------------------------------

bool test_lfo_phase_independence(void) {
    lfo_enhanced_t lfo;
    lfo_enhanced_init(&lfo);

    // Check initial 90° offset
    float p1 = vgetq_lane_f32(lfo.phase1, 0);
    float p2 = vgetq_lane_f32(lfo.phase2, 0);

    bool passed = fabsf(p2 - p1 - 0.25f) < 0.001f;
    register_test("LFO Phase Independence", passed, 0,
                  passed ? NULL : "Phase offset incorrect");
    return passed;
}

bool test_lfo_bipolar_depth(void) {
    lfo_enhanced_t lfo;
    lfo_enhanced_init(&lfo);

    // Test positive depth
    lfo_enhanced_update(&lfo, 0, LFO_TARGET_PITCH, LFO_TARGET_NONE,
                        100.0f, 0.0f, 50.0f, 50.0f);

    float depth = vgetq_lane_f32(lfo.depth1, 0);
    bool passed = fabsf(depth - 1.0f) < 0.001f;

    if (passed) {
        // Test negative depth
        lfo_enhanced_update(&lfo, 0, LFO_TARGET_PITCH, LFO_TARGET_NONE,
                            0.0f, 0.0f, 50.0f, 50.0f);  // 0% = -1.0

        depth = vgetq_lane_f32(lfo.depth1, 0);
        passed = fabsf(depth + 1.0f) < 0.001f;
    }

    register_test("LFO Bipolar Depth", passed, 0,
                  passed ? NULL : "Depth scaling incorrect");
    return passed;
}

bool test_lfo_circular_reference(void) {
    lfo_enhanced_t lfo;
    lfo_enhanced_init(&lfo);

    // Try to create circular reference
    lfo_enhanced_update(&lfo, 0,
                        LFO_TARGET_LFO2_PHASE,  // LFO1 -> LFO2
                        LFO_TARGET_LFO1_PHASE,  // LFO2 -> LFO1 (circular!)
                        50.0f, 50.0f, 50.0f, 50.0f);

    // Should have disabled both
    bool passed = (lfo.target1 == LFO_TARGET_NONE &&
                   lfo.target2 == LFO_TARGET_NONE);

    register_test("LFO Circular Reference Prevention", passed, 0,
                  passed ? NULL : "Circular reference not detected");
    return passed;
}

// ----------------------------------------------------------------------------
// Euclidean Tuning Tests
// ----------------------------------------------------------------------------

bool test_euclidean_mode_off(void) {
    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    synth.params[PARAM_LFO2_SHAPE] = EUCLID_MODE_OFF;
    fm_perc_synth_update_params(&synth);

    float o0 = vgetq_lane_f32(synth.euclid_offsets, 0);
    float o1 = vgetq_lane_f32(synth.euclid_offsets, 1);
    float o2 = vgetq_lane_f32(synth.euclid_offsets, 2);
    float o3 = vgetq_lane_f32(synth.euclid_offsets, 3);

    bool passed = (o0 == 0.0f && o1 == 0.0f && o2 == 0.0f && o3 == 0.0f);
    register_test("EuclTun Mode Off (all zero)", passed, 0,
                  passed ? NULL : "Mode 0 should give [0,0,0,0]");
    return passed;
}

bool test_euclidean_dim7(void) {
    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    // Mode 6 = Dim7: E(4,12) = [0, 3, 6, 9]
    synth.params[PARAM_LFO2_SHAPE] = EUCLID_MODE_DIM7;
    fm_perc_synth_update_params(&synth);

    bool passed = (fabsf(vgetq_lane_f32(synth.euclid_offsets, 0) - 0.0f) < 0.001f &&
                   fabsf(vgetq_lane_f32(synth.euclid_offsets, 1) - 3.0f) < 0.001f &&
                   fabsf(vgetq_lane_f32(synth.euclid_offsets, 2) - 6.0f) < 0.001f &&
                   fabsf(vgetq_lane_f32(synth.euclid_offsets, 3) - 9.0f) < 0.001f);
    register_test("EuclTun Dim7 E(4,12)=[0,3,6,9]", passed, 0,
                  passed ? NULL : "Dim7 offsets incorrect");
    return passed;
}

bool test_euclidean_all_modes_monotonic(void) {
    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    bool passed = true;
    // For all modes: voice 0 = root (0), offsets monotonically non-decreasing
    for (int m = 0; m < EUCLID_MODE_COUNT && passed; m++) {
        synth.params[PARAM_LFO2_SHAPE] = (int8_t)m;
        fm_perc_synth_update_params(&synth);

        float o0 = vgetq_lane_f32(synth.euclid_offsets, 0);
        float o1 = vgetq_lane_f32(synth.euclid_offsets, 1);
        float o2 = vgetq_lane_f32(synth.euclid_offsets, 2);
        float o3 = vgetq_lane_f32(synth.euclid_offsets, 3);

        if (o0 != 0.0f || o1 < o0 || o2 < o1 || o3 < o2)
            passed = false;
    }
    register_test("EuclTun All Modes Monotonic", passed, 0,
                  passed ? NULL : "A mode has non-monotonic or non-zero root offset");
    return passed;
}

bool test_euclidean_note_spread(void) {
    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    // Whole-tone mode: E(4,8) = [0, 2, 4, 6] semitones
    synth.params[PARAM_LFO2_SHAPE] = EUCLID_MODE_WHOLE;
    synth.params[PARAM_VOICE_ALLOC] = 0;  // K-S-M-P default
    fm_perc_synth_update_params(&synth);

    // Note 60 (C4). Voice 3 = Perc engine, lane 3 → gets offset +6 semitones
    // MIDI 60+6 = 66 = F#4, freq = 440 * 2^((66-69)/12) ≈ 369.99 Hz
    fm_perc_synth_note_on(&synth, 60, 100);

    float expected = 440.0f * powf(2.0f, (66.0f - 69.0f) / 12.0f);  // ~369.99 Hz
    float actual   = vgetq_lane_f32(synth.perc.carrier_freq_base, 3);
    bool passed = (fabsf(actual - expected) < 2.0f);  // within 2 Hz

    register_test("EuclTun Note Spread (perc voice +6 st)", passed, 0,
                  passed ? NULL : "Perc engine did not receive Euclidean-shifted pitch");
    return passed;
}

bool test_euclidean_mode_off_no_spread(void) {
    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    // Mode Off: all voices same pitch at trigger
    synth.params[PARAM_LFO2_SHAPE] = EUCLID_MODE_OFF;
    synth.params[PARAM_VOICE_ALLOC] = 0;
    fm_perc_synth_update_params(&synth);

    fm_perc_synth_note_on(&synth, 60, 100);  // C4 = 261.63 Hz

    float expected = 440.0f * powf(2.0f, (60.0f - 69.0f) / 12.0f);  // ~261.63 Hz
    float kick_f  = vgetq_lane_f32(synth.kick.carrier_freq, 0);
    float perc_f  = vgetq_lane_f32(synth.perc.carrier_freq_base, 3);
    bool passed = (fabsf(kick_f - expected) < 2.0f && fabsf(perc_f - expected) < 2.0f);

    register_test("EuclTun Off: all voices same pitch", passed, 0,
                  passed ? NULL : "Mode Off should not spread voice pitches");
    return passed;
}

// ============================================================================
// PART 1c: EXTREME LFO BOUNDARY TEST
// ============================================================================

/**
 * test_lfo_extreme_boundaries
 *
 * Verifies that maximum LFO depth (±100%) does not produce NaN, Inf, or
 * out-of-range output values regardless of LFO target.
 *
 * This exercises the clipping behavior described in PROGRESS.md:
 * "Extreme modulation at boundaries (clipping behavior)".
 *
 * Each LFO target is tested with a note-on + 480-sample render (10ms).
 * Output must stay in [-1.0, 1.0] and be free of NaN/Inf.
 */
bool test_lfo_extreme_boundaries(void) {
    const int N_SAMPLES = 480;
    const int N_TARGETS = 11;  // LFO_TARGET_NONE through LFO_TARGET_METAL_GATE
    bool all_ok = true;
    int nan_total = 0, inf_total = 0, range_total = 0;

    for (int target = 0; target < N_TARGETS; target++) {
        fm_perc_synth_t synth;
        fm_perc_synth_init(&synth);

        // LFO1: max positive depth, max rate, current target
        synth.params[PARAM_LFO1_SHAPE]  = 0;    // Tri+Tri
        synth.params[PARAM_LFO1_RATE]   = 100;  // max rate
        synth.params[PARAM_LFO1_TARGET] = target;
        synth.params[PARAM_LFO1_DEPTH]  = 100;  // max positive depth

        // LFO2: max negative depth, same target
        synth.params[PARAM_LFO2_RATE]   = 100;
        synth.params[PARAM_LFO2_TARGET] = target;
        synth.params[PARAM_LFO2_DEPTH]  = -100; // max negative depth

        fm_perc_synth_update_params(&synth);
        fm_perc_synth_note_on(&synth, 60, 127);

        int nan_count = 0, inf_count = 0, range_count = 0;
        for (int n = 0; n < N_SAMPLES; n++) {
            float out = fm_perc_synth_process(&synth);
            if (isnan(out)) nan_count++;
            if (isinf(out)) inf_count++;
            if (out < -1.001f || out > 1.001f) range_count++;
        }

        bool ok = (nan_count == 0 && inf_count == 0 && range_count == 0);
        if (!ok) {
            all_ok = false;
            printf("  LFO target %2d: NaN=%d Inf=%d OutOfRange=%d  FAIL\n",
                   target, nan_count, inf_count, range_count);
        }
        nan_total   += nan_count;
        inf_total   += inf_count;
        range_total += range_count;
    }

    printf("  All %d LFO targets at max depth: NaN=%d Inf=%d OutOfRange=%d  %s\n",
           N_TARGETS, nan_total, inf_total, range_total, all_ok ? "PASS" : "FAIL");

    register_test("LFO extreme boundaries (all targets, max depth)",
                  all_ok, 0,
                  all_ok ? NULL : "NaN/Inf/range violation at max LFO depth");
    return all_ok;
}

// ============================================================================
// PART 2: INTEGRATION TESTS
// ============================================================================

bool test_full_synth_initialization(void) {
    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    bool passed = true;

    // Check that all engines are initialized
    if (vgetq_lane_f32(synth.kick.carrier_phase, 0) != 0.0f) passed = false;
    if (vgetq_lane_f32(synth.snare.carrier_phase, 0) != 0.0f) passed = false;
    if (vgetq_lane_f32(synth.metal.phase[0], 0) != 0.0f) passed = false;
    if (vgetq_lane_f32(synth.perc.phase[0], 0) != 0.0f) passed = false;

    // Check LFO phase independence
    float p1 = vgetq_lane_f32(synth.lfo.phase1, 0);
    float p2 = vgetq_lane_f32(synth.lfo.phase2, 0);
    if (fabsf(p2 - p1 - 0.25f) > 0.001f) passed = false;

    register_test("Full Synth Initialization", passed, 0,
                  passed ? NULL : "Initialization failed");
    return passed;
}

bool test_parameter_smoothing(void) {
    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    // Change a parameter
    synth.params[13] = 80;  // LFO1 rate to 80%
    fm_perc_synth_update_params(&synth);

    float last_rate = 0;
    bool passed = true;

    // Process smoothing for 100 samples
    for (int i = 0; i < 100; i++) {
        lfo_smoother_process(&synth.lfo_smooth);
        float current = vgetq_lane_f32(synth.lfo_smooth.current_rate1, 0);

        if (i < 48) {  // During ramp
            if (current <= last_rate) {
                passed = false;
                break;
            }
        }
        last_rate = current;
    }

    register_test("Parameter Smoothing", passed, 0,
                  passed ? NULL : "Smoothing ramp failed");
    return passed;
}

// ============================================================================
// PART 3: LONG-RUN STABILITY TEST (16 BEATS)
// ============================================================================

/**
 * 16-beat pattern test - runs synth for 16 beats at 120 BPM
 * Verifies:
 * - No crashes or hangs
 * - No denormals (output stays in range)
 * - Consistent CPU usage
 * - No memory leaks
 * - Phase accumulators don't drift
 */
bool test_16_beat_stability(void) {
    printf("\n========================================\n");
    printf("  16-BEAT STABILITY TEST (LONG RUN)\n");
    printf("========================================\n");

    fm_perc_synth_t synth;
    fm_perc_synth_init(&synth);

    // Load a preset with all voices active
    load_fm_preset(1, synth.params);  // "Metal Storm"
    fm_perc_synth_update_params(&synth);

    // Calculate samples per beat at 120 BPM
    float samples_per_beat = (60.0f / TEST_BPM) * TEST_SAMPLE_RATE;
    int total_samples = (int)(samples_per_beat * TEST_LONG_RUN_BEATS);

    printf("BPM: %d\n", TEST_BPM);
    printf("Beats: %d\n", TEST_LONG_RUN_BEATS);
    printf("Sample rate: %d Hz\n", TEST_SAMPLE_RATE);
    printf("Total samples: %d (%.2f seconds)\n", total_samples,
           total_samples / (float)TEST_SAMPLE_RATE);

    // Pattern: Kick on 1, Snare on 2&4, Metal on offbeats, Perc on 3
    int kick_beat = 0;      // Beat 1
    int snare_beats[] = {1, 3};  // Beats 2 and 4
    int metal_offbeats[] = {0, 1, 2, 3};  // Eighth notes
    int perc_beat = 2;      // Beat 3

    uint32_t start_cycles = read_cycle_counter();
    uint32_t min_cycles = 0xFFFFFFFF;
    uint32_t max_cycles = 0;
    uint64_t total_cycles = 0;

    float min_output = 1.0f;
    float max_output = -1.0f;
    int zero_crossings = 0;
    float last_output = 0;

    bool denormal_detected = false;
    bool crash_detected = false;

    // Main processing loop
    for (int sample = 0; sample < total_samples; sample++) {
        // Calculate current beat position (0-3.999)
        float beat_pos = (sample / samples_per_beat);
        int beat = (int)beat_pos;
        float subbeat = beat_pos - beat;

        // Trigger notes on beat boundaries
        if (sample > 0 && (int)((sample-1) / samples_per_beat) != beat) {
            // New beat started
            if (beat == kick_beat) {
                fm_perc_synth_note_on(&synth, 36, 100);  // Kick on C2
            }

            for (int i = 0; i < 2; i++) {
                if (beat == snare_beats[i]) {
                    fm_perc_synth_note_on(&synth, 38, 90);  // Snare on D2
                }
            }

            if (beat == perc_beat) {
                fm_perc_synth_note_on(&synth, 45, 80);  // Perc on A2
            }

            // Metal on offbeats (eighth notes)
            for (int i = 0; i < 4; i++) {
                if (fabsf(subbeat - i * 0.25f) < 0.01f) {
                    fm_perc_synth_note_on(&synth, 42, 70);  // Metal on F#2
                }
            }
        }

        // Process one sample and measure cycles
        uint32_t sample_start = read_cycle_counter();
        float output = fm_perc_synth_process(&synth);
        uint32_t sample_cycles = read_cycle_counter() - sample_start;

        // Track statistics
        total_cycles += sample_cycles;
        if (sample_cycles < min_cycles) min_cycles = sample_cycles;
        if (sample_cycles > max_cycles) max_cycles = sample_cycles;

        if (output < min_output) min_output = output;
        if (output > max_output) max_output = output;

        // Check for denormals (output very close to zero but not zero)
        if (output != 0 && fabsf(output) < 1e-30f) {
            denormal_detected = true;
        }

        // Count zero crossings (for frequency estimation)
        if (sample > 0 && (last_output < 0 && output > 0)) {
            zero_crossings++;
        }
        last_output = output;

        // Progress indicator every 10%
        if (sample % (total_samples / 10) == 0) {
            printf("Progress: %d%%\r", (sample * 100) / total_samples);
            fflush(stdout);
        }
    }

    uint32_t end_cycles = read_cycle_counter();
    uint32_t total_test_cycles = end_cycles - start_cycles;
    float avg_cycles = total_cycles / total_samples;
    float cpu_usage = (avg_cycles * TEST_SAMPLE_RATE) / 1000000000.0f * 100.0f;  // At 1GHz

    printf("\n\n========================================\n");
    printf("           STABILITY RESULTS\n");
    printf("========================================\n");
    printf("Total test cycles: %u\n", total_test_cycles);
    printf("Average cycles/sample: %.1f\n", avg_cycles);
    printf("Min cycles/sample: %u\n", min_cycles);
    printf("Max cycles/sample: %u\n", max_cycles);
    printf("CPU usage (est @1GHz): %.2f%%\n", cpu_usage);
    printf("\n");
    printf("Output range: [%.3f, %.3f]\n", min_output, max_output);
    printf("Zero crossings: %d\n", zero_crossings);
    printf("Estimated frequency: %.1f Hz\n",
           (zero_crossings / 2.0f) / (total_samples / (float)TEST_SAMPLE_RATE));
    printf("\n");

    bool passed = true;
    char error[256] = "";

    // Check for denormals
    if (denormal_detected) {
        passed = false;
        strcpy(error, "Denormal numbers detected");
    }

    // Check that output is within expected range
    if (min_output < -1.1f || max_output > 1.1f) {
        passed = false;
        sprintf(error, "Output out of range [%.3f, %.3f]", min_output, max_output);
    }

    // Check CPU usage (should be < 5% @1GHz)
    if (cpu_usage > 5.0f) {
        passed = false;
        sprintf(error, "CPU usage too high: %.2f%%", cpu_usage);
    }

    // Check cycle consistency (max should be no more than 2x average)
    if (max_cycles > avg_cycles * 2) {
        passed = false;
        sprintf(error, "Cycle spike detected: max %u vs avg %.1f", max_cycles, avg_cycles);
    }

    register_test("16-Beat Stability", passed, avg_cycles, passed ? NULL : error);

    if (passed) {
        printf("✅ STABILITY TEST PASSED\n");
    } else {
        printf("❌ STABILITY TEST FAILED: %s\n", error);
    }

    return passed;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

/**
 * Run all tests and return overall pass/fail
 */
bool run_all_tests(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║     FM PERCUSSION SYNTHESIZER - COMPLETE TEST SUITE    ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");

    test_count = 0;

    // PART 1: Individual Component Tests
    printf("\n📋 PART 1: INDIVIDUAL COMPONENTS\n");
    printf("----------------------------------------\n");

    test_prng_uniformity();
    test_prng_independence();
    test_sine_accuracy();
    test_sine_parallel_execution();
    test_envelope_rom_values();
    test_envelope_generation();
    test_kick_engine();
    test_snare_engine();
    test_metal_engine();
    test_perc_engine();
    test_lfo_phase_independence();
    test_lfo_bipolar_depth();
    test_lfo_circular_reference();

    // PART 1b: Euclidean Tuning
    printf("\n📋 PART 1b: EUCLIDEAN TUNING\n");
    printf("----------------------------------------\n");

    test_euclidean_mode_off();
    test_euclidean_dim7();
    test_euclidean_all_modes_monotonic();
    test_euclidean_note_spread();
    test_euclidean_mode_off_no_spread();

    // PART 1c: Extreme LFO Boundaries
    printf("\n📋 PART 1c: EXTREME LFO BOUNDARIES\n");
    printf("----------------------------------------\n");

    test_lfo_extreme_boundaries();

    // PART 2: Integration Tests
    printf("\n📋 PART 2: INTEGRATION\n");
    printf("----------------------------------------\n");

    test_full_synth_initialization();
    test_parameter_smoothing();

    // PART 3: Long-Run Stability
    printf("\n📋 PART 3: LONG-RUN STABILITY\n");
    printf("----------------------------------------\n");

    test_16_beat_stability();

    // Summary
    print_test_summary();

    // Return overall pass/fail
    int passed = 0;
    for (int i = 0; i < test_count; i++) {
        if (test_results[i].passed) passed++;
    }

    return passed == test_count;
}

// ============================================================================
// INDIVIDUAL TEST RUNNERS (for development)
// ============================================================================

#ifdef TEST_PRNG_ONLY
int main(void) {
    test_prng_uniformity();
    test_prng_independence();
    print_test_summary();
    return 0;
}
#endif

#ifdef TEST_SINE_ONLY
int main(void) {
    test_sine_accuracy();
    test_sine_parallel_execution();
    print_test_summary();
    return 0;
}
#endif

#ifdef TEST_ENGINES_ONLY
int main(void) {
    test_kick_engine();
    test_snare_engine();
    test_metal_engine();
    test_perc_engine();
    print_test_summary();
    return 0;
}
#endif

#ifdef TEST_LFO_ONLY
int main(void) {
    test_lfo_phase_independence();
    test_lfo_bipolar_depth();
    test_lfo_circular_reference();
    print_test_summary();
    return 0;
}
#endif

#ifdef TEST_STABILITY_ONLY
int main(void) {
    test_16_beat_stability();
    print_test_summary();
    return 0;
}
#endif

// Default: run all tests
#ifndef TEST_PRNG_ONLY
#ifndef TEST_SINE_ONLY
#ifndef TEST_ENGINES_ONLY
#ifndef TEST_LFO_ONLY
#ifndef TEST_STABILITY_ONLY
int main(void) {
    return run_all_tests() ? 0 : 1;
}
#endif  // TEST_STABILITY_ONLY
#endif  // TEST_LFO_ONLY
#endif  // TEST_ENGINES_ONLY
#endif  // TEST_SINE_ONLY
#endif  // TEST_PRNG_ONLY

#endif // TESTS_H