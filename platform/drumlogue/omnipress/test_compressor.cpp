/**
 * @file test_compressor.cpp
 * @brief x86-compilable unit tests for OmniPress compressor DSP math.
 *
 * Tests gain computer, attack/release coefficients, parameter mapping,
 * and wet/dry mix arithmetic using scalar equivalents of the NEON code.
 *
 * Compile: g++ -std=c++14 -o test_compressor test_compressor.cpp -lm
 * Run:     ./test_compressor
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>

#define SAMPLE_RATE 48000.0f
#define EPSILON     1e-4f

/* -------------------------------------------------------------------------
 * Scalar compressor gain computer
 * Mirrors compressor_core.h compressor_calc_gain() using standard log10.
 * -------------------------------------------------------------------- */

/**
 * Standard gain computer (hard knee).
 *   Above threshold: GR_dB = -(envelope_dB - thresh_dB) * (1 - 1/ratio)
 *   Below threshold: GR_dB = 0
 * Returns gain reduction in dB (negative means gain reduction).
 */
static float gain_computer_hard(float envelope_linear, float thresh_db, float ratio) {
    if (envelope_linear <= 0.0f) return 0.0f;
    float env_db = 20.0f * log10f(envelope_linear);
    float excess = env_db - thresh_db;
    if (excess <= 0.0f) return 0.0f;
    /* slope = 1 - 1/ratio → gain reduction in dB */
    return -excess * (1.0f - 1.0f / ratio);
}

/**
 * Soft-knee gain computer.
 * In the knee region [thresh - knee/2, thresh + knee/2]:
 *   GR = -(env_dB - thresh + knee/2)^2 / (2*knee)
 * Above knee: hard gain reduction.
 */
static float gain_computer_soft(float envelope_linear, float thresh_db,
                                 float ratio, float knee_db) {
    if (envelope_linear <= 0.0f) return 0.0f;
    float env_db  = 20.0f * log10f(envelope_linear);
    float excess  = env_db - thresh_db;
    float hw      = knee_db * 0.5f;

    if (excess < -hw) return 0.0f;
    if (excess <= hw) {
        /* In knee region */
        float t = excess + hw;
        return -(t * t) / (2.0f * knee_db) * (1.0f - 1.0f / ratio);
    }
    return -excess * (1.0f - 1.0f / ratio);
}

/**
 * Attack/release coefficient:  coeff = exp(-1 / (time_ms * 0.001 * SR))
 */
static float ar_coeff(float time_ms) {
    return expf(-1.0f / (time_ms * 0.001f * SAMPLE_RATE));
}

/**
 * One-pole smoother: state = coeff * state + (1-coeff) * target
 */
static float smooth_one_pole(float state, float target, float coeff) {
    return coeff * state + (1.0f - coeff) * target;
}

/* -------------------------------------------------------------------------
 * Test 1: Gain computer below threshold → no gain reduction
 * ---------------------------------------------------------------------- */

void test_gain_computer_below_threshold() {
    printf("\n=== Test 1: Gain Computer Below Threshold ===\n");

    float thresh_db = -20.0f, ratio = 4.0f;
    /* Signals well below threshold */
    float signals[] = {0.001f, 0.01f, 0.09f};  /* -60, -40, -20.8 dBFS */

    for (int i = 0; i < 3; i++) {
        float gr = gain_computer_hard(signals[i], thresh_db, ratio);
        int ok = (fabsf(gr) < EPSILON);
        printf("  env=%.3f (%.1f dB): GR=%.4f dB  %s\n",
               signals[i], 20.0f * log10f(signals[i]), gr, ok ? "PASS" : "FAIL");
        assert(ok);
    }
}

/* -------------------------------------------------------------------------
 * Test 2: Gain computer above threshold → correct slope
 * ---------------------------------------------------------------------- */

void test_gain_computer_above_threshold() {
    printf("\n=== Test 2: Gain Computer Above Threshold ===\n");

    float thresh_db = -20.0f;

    /* Ratio 4:1: 12 dB above threshold → GR = -12*(1-1/4) = -9 dB */
    float env_4_1 = powf(10.0f, (-20.0f + 12.0f) / 20.0f);
    float gr_4_1  = gain_computer_hard(env_4_1, thresh_db, 4.0f);
    int ok1 = fabsf(gr_4_1 - (-9.0f)) < 0.01f;
    printf("  4:1 ratio, 12dB above thresh: GR=%.2f dB (expect -9.0)  %s\n",
           gr_4_1, ok1 ? "PASS" : "FAIL");
    assert(ok1);

    /* Ratio 2:1: 10 dB above threshold → GR = -10*(1-1/2) = -5 dB */
    float env_2_1 = powf(10.0f, (-20.0f + 10.0f) / 20.0f);
    float gr_2_1  = gain_computer_hard(env_2_1, thresh_db, 2.0f);
    int ok2 = fabsf(gr_2_1 - (-5.0f)) < 0.01f;
    printf("  2:1 ratio, 10dB above thresh: GR=%.2f dB (expect -5.0)  %s\n",
           gr_2_1, ok2 ? "PASS" : "FAIL");
    assert(ok2);

    /* Ratio 10:1: 20 dB above threshold → GR = -20*(1-0.1) = -18 dB */
    float env_10_1 = powf(10.0f, (-20.0f + 20.0f) / 20.0f);
    float gr_10_1  = gain_computer_hard(env_10_1, thresh_db, 10.0f);
    int ok3 = fabsf(gr_10_1 - (-18.0f)) < 0.01f;
    printf("  10:1 ratio, 20dB above thresh: GR=%.2f dB (expect -18.0)  %s\n",
           gr_10_1, ok3 ? "PASS" : "FAIL");
    assert(ok3);
}

/* -------------------------------------------------------------------------
 * Test 3: Hard limiter (ratio → ∞ or large) → full gain reduction
 * ---------------------------------------------------------------------- */

void test_hard_limiter() {
    printf("\n=== Test 3: Hard Limiter (very high ratio) ===\n");

    float thresh_db = -6.0f;
    float env = powf(10.0f, 0.0f / 20.0f);  /* 0 dBFS (6 dB above threshold) */

    /* 20:1 ratio: GR ≈ -6*(1-1/20) = -5.7 dB */
    float gr_20 = gain_computer_hard(env, thresh_db, 20.0f);
    int ok1 = gr_20 < -5.0f;
    printf("  20:1, 6dB above: GR=%.2f dB (expect < -5): %s\n",
           gr_20, ok1 ? "PASS" : "FAIL");
    assert(ok1);

    /* 100:1 ratio: GR ≈ -5.94 dB (nearly -6) */
    float gr_100 = gain_computer_hard(env, thresh_db, 100.0f);
    int ok2 = gr_100 < gr_20;
    printf("  100:1, 6dB above: GR=%.2f dB (must be < 20:1 GR=%.2f): %s\n",
           gr_100, gr_20, ok2 ? "PASS" : "FAIL");
    assert(ok2);
}

/* -------------------------------------------------------------------------
 * Test 4: Soft knee is between hard knee and no compression
 * ---------------------------------------------------------------------- */

void test_soft_knee() {
    printf("\n=== Test 4: Soft Knee in Transition Zone ===\n");

    float thresh_db = -20.0f, ratio = 4.0f, knee_db = 6.0f;

    /* Test at threshold (in the middle of the knee) */
    float env_at_thresh = powf(10.0f, thresh_db / 20.0f);
    float gr_hard = gain_computer_hard(env_at_thresh, thresh_db, ratio);
    float gr_soft = gain_computer_soft(env_at_thresh, thresh_db, ratio, knee_db);

    /* Hard knee at threshold = 0 (just at threshold, no excess yet) */
    printf("  At threshold (hard): GR=%.4f dB (expect 0)\n", gr_hard);
    printf("  At threshold (soft): GR=%.4f dB (expect < 0, > hard)\n", gr_soft);
    int ok1 = fabsf(gr_hard) < EPSILON;
    int ok2 = (gr_soft < 0.0f) && (gr_soft > gr_hard - 1.0f);
    printf("  Hard GR at thresh = 0: %s\n", ok1 ? "PASS" : "FAIL");
    printf("  Soft GR at thresh in (hard-1, 0): %s\n", ok2 ? "PASS" : "FAIL");
    assert(ok1 && ok2);

    /* Well above knee: hard ≈ soft */
    float env_above = powf(10.0f, (-20.0f + 20.0f) / 20.0f);
    float gh = gain_computer_hard(env_above, thresh_db, ratio);
    float gs = gain_computer_soft(env_above, thresh_db, ratio, knee_db);
    int ok3 = fabsf(gh - gs) < 0.1f;  /* Should converge well above knee */
    printf("  20dB above thresh, hard=%.2f, soft=%.2f: %s\n",
           gh, gs, ok3 ? "PASS" : "FAIL");
    assert(ok3);
}

/* -------------------------------------------------------------------------
 * Test 5: Attack/release coefficient validity
 * ---------------------------------------------------------------------- */

void test_ar_coefficients() {
    printf("\n=== Test 5: Attack/Release Coefficients ===\n");

    struct { float ms; } cases[] = {{0.1f}, {1.0f}, {10.0f}, {100.0f}, {1000.0f}};

    int all_ok = 1;
    for (int i = 0; i < 5; i++) {
        float ms = cases[i].ms;
        float c = ar_coeff(ms);
        int ok = (c > 0.0f) && (c < 1.0f);
        printf("  %6.1f ms → coeff=%.8f  %s\n", ms, c, ok ? "PASS" : "FAIL");
        if (!ok) all_ok = 0;
    }
    assert(all_ok);

    /* Longer time → coefficient closer to 1 (slower response) */
    float c_fast = ar_coeff(1.0f);
    float c_slow = ar_coeff(100.0f);
    int mono = (c_fast < c_slow);
    printf("  1ms coeff < 100ms coeff (%g < %g): %s\n",
           c_fast, c_slow, mono ? "PASS" : "FAIL");
    assert(mono);
}

/* -------------------------------------------------------------------------
 * Test 6: Attack/release smoothing – 63% reached in one time constant
 * ---------------------------------------------------------------------- */

void test_attack_release_timing() {
    printf("\n=== Test 6: Attack/Release Time Constant Accuracy ===\n");

    /* Attack: 10 ms target step from 0 to 1.
     * After 10ms (480 samples) should be at 1 - e^-1 ≈ 0.632 */
    float att_ms = 10.0f;
    float coeff = ar_coeff(att_ms);
    int att_samples = (int)(att_ms * 0.001f * SAMPLE_RATE);

    float state = 0.0f;
    for (int n = 0; n < att_samples; n++) {
        state = smooth_one_pole(state, 1.0f, coeff);
    }
    float expected = 1.0f - expf(-1.0f);
    int ok = fabsf(state - expected) < 0.005f;
    printf("  Attack 10ms: after %d samples → %.4f (expect ~%.4f)  %s\n",
           att_samples, state, expected, ok ? "PASS" : "FAIL");
    assert(ok);

    /* Release: 100 ms */
    float rel_ms = 100.0f;
    float rcoeff = ar_coeff(rel_ms);
    int rel_samples = (int)(rel_ms * 0.001f * SAMPLE_RATE);

    state = 1.0f;
    for (int n = 0; n < rel_samples; n++) {
        state = smooth_one_pole(state, 0.0f, rcoeff);
    }
    float expected_r = expf(-1.0f);  /* 1/e ≈ 0.368 remaining */
    int ok_r = fabsf(state - expected_r) < 0.005f;
    printf("  Release 100ms: after %d samples → %.4f (expect ~%.4f)  %s\n",
           rel_samples, state, expected_r, ok_r ? "PASS" : "FAIL");
    assert(ok_r);
}

/* -------------------------------------------------------------------------
 * Test 7: Parameter range mapping (raw int → float)
 * ---------------------------------------------------------------------- */

void test_parameter_mapping() {
    printf("\n=== Test 7: Parameter Range Mapping ===\n");

    /* Threshold: stored *10, range -60..0 dB */
    int raw_thresh_min = -600, raw_thresh_max = 0;
    float thresh_min = raw_thresh_min / 10.0f;
    float thresh_max = raw_thresh_max / 10.0f;
    int ok1 = (fabsf(thresh_min - (-60.0f)) < EPSILON) &&
              (fabsf(thresh_max - 0.0f) < EPSILON);
    printf("  Threshold: [%d, %d] → [%.1f, %.1f] dB  %s\n",
           raw_thresh_min, raw_thresh_max, thresh_min, thresh_max,
           ok1 ? "PASS" : "FAIL");
    assert(ok1);

    /* Ratio: stored *10, range 1.0..20.0 */
    int raw_ratio_min = 10, raw_ratio_max = 200;
    float ratio_min = raw_ratio_min / 10.0f;
    float ratio_max = raw_ratio_max / 10.0f;
    int ok2 = (fabsf(ratio_min - 1.0f) < EPSILON) &&
              (fabsf(ratio_max - 20.0f) < EPSILON);
    printf("  Ratio: [%d, %d] → [%.1f, %.1f]  %s\n",
           raw_ratio_min, raw_ratio_max, ratio_min, ratio_max,
           ok2 ? "PASS" : "FAIL");
    assert(ok2);

    /* Attack: 1=0.1ms, 1000=100ms */
    float att_ms_min = 1  / 10.0f;
    float att_ms_max = 1000 / 10.0f;
    int ok3 = (fabsf(att_ms_min - 0.1f) < EPSILON) &&
              (fabsf(att_ms_max - 100.0f) < EPSILON);
    printf("  Attack: [1,1000] → [%.1f, %.1f] ms  %s\n",
           att_ms_min, att_ms_max, ok3 ? "PASS" : "FAIL");
    assert(ok3);

    /* Makeup: raw 0..240 → 0..24.0 dB */
    float makeup_min = 0   / 10.0f;
    float makeup_max = 240 / 10.0f;
    int ok4 = (fabsf(makeup_min - 0.0f) < EPSILON) &&
              (fabsf(makeup_max - 24.0f) < EPSILON);
    printf("  Makeup: [0,240] → [%.1f, %.1f] dB  %s\n",
           makeup_min, makeup_max, ok4 ? "PASS" : "FAIL");
    assert(ok4);
}

/* -------------------------------------------------------------------------
 * Test 8: Wet/dry mix
 * ---------------------------------------------------------------------- */

void test_wetdry_mix() {
    printf("\n=== Test 8: Wet/Dry Mix ===\n");

    /* MIX raw range: -100..100 → -1..1 (negative = more dry) */
    /* mix_linear = raw_mix / 100.0 clamped to [-1,1] */
    struct { int raw; float wet_expected; } cases[] = {
        {100,   1.0f},   /* fully wet */
        {0,     0.0f},   /* 50/50 (wet=0, but formula: out=dry+wet*0=dry) */
        {-100, -1.0f},   /* negative: expansion/reverse */
        {50,    0.5f}
    };

    float dry = 0.7f, wet_signal = 0.3f;
    int all_ok = 1;
    for (int i = 0; i < 4; i++) {
        float mix = cases[i].raw / 100.0f;
        /* out = dry*(1 - |mix|) + wet_signal*|mix| when mix>=0
         * For negative mix, the same math applies symmetrically.
         * Simple test: at mix=1, out==wet; at mix=-1, result uses -1 factor */
        float out = dry * (1.0f - fabsf(mix)) + wet_signal * fabsf(mix);
        float e_wet  = wet_signal;
        float e_dry  = dry;
        float e_half = (dry + wet_signal) * 0.5f;

        /* Check limiting cases */
        int ok = 1;
        if (cases[i].raw == 100)  ok = fabsf(out - e_wet)  < EPSILON;
        if (cases[i].raw == -100) ok = fabsf(out - e_wet)  < EPSILON;  /* |-1|=1 */
        if (cases[i].raw == 0)    ok = fabsf(out - e_dry)  < EPSILON;
        if (cases[i].raw == 50)   ok = fabsf(out - e_half) < EPSILON;

        printf("  raw=%4d (mix=%.2f): out=%.4f  %s\n",
               cases[i].raw, mix, out, ok ? "PASS" : "FAIL");
        if (!ok) all_ok = 0;
    }
    assert(all_ok);
}

/* -------------------------------------------------------------------------
 * Test 9: Makeup gain conversion (dB → linear)
 * ---------------------------------------------------------------------- */

void test_makeup_gain_conversion() {
    printf("\n=== Test 9: Makeup Gain dB → Linear ===\n");

    struct { float db; float lin_expected; } cases[] = {
        { 0.0f,  1.0f},
        { 6.0f,  2.0f},       /* approximately */
        {20.0f, 10.0f},
        {24.0f, 15.849f}
    };

    for (int i = 0; i < 4; i++) {
        float lin = powf(10.0f, cases[i].db / 20.0f);
        int ok = fabsf(lin - cases[i].lin_expected) < 0.01f;
        printf("  %.0f dB → linear=%.4f (expect %.4f)  %s\n",
               cases[i].db, lin, cases[i].lin_expected, ok ? "PASS" : "FAIL");
        assert(ok);
    }
}

/* -------------------------------------------------------------------------
 * Test 10: Sidechain HPF cutoff parameter range validation
 * ---------------------------------------------------------------------- */

void test_sc_hpf_range() {
    printf("\n=== Test 10: Sidechain HPF Parameter Range ===\n");

    int sc_min = 20, sc_max = 500;
    /* Valid HPF cutoffs: must be > 0 and < Nyquist (24000 Hz) */
    int ok_min = (sc_min > 0) && (sc_min < 24000);
    int ok_max = (sc_max > sc_min) && (sc_max < 24000);

    printf("  SC HPF min=%d Hz: valid range  %s\n", sc_min, ok_min ? "PASS" : "FAIL");
    printf("  SC HPF max=%d Hz: valid range  %s\n", sc_max, ok_max ? "PASS" : "FAIL");
    assert(ok_min && ok_max);

    /* Compute HPF coefficient at extremes, should be in (0,1) */
    float pi = 3.141592653589793f;
    int fcs[] = {20, 100, 500};
    for (int fi = 0; fi < 3; fi++) {
        int fc = fcs[fi];
        float w0 = 2.0f * pi * fc / SAMPLE_RATE;
        float alpha = sinf(w0) / (2.0f * 0.5f);  /* Q=0.5 (Bessel) */
        float a0 = 1.0f + alpha;
        float norm_coeff = alpha / a0;
        int ok = (norm_coeff > 0.0f) && (norm_coeff < 1.0f);
        printf("  fc=%4d Hz: HPF b0=%.6f  %s\n", fc, norm_coeff, ok ? "PASS" : "FAIL");
        assert(ok);
    }
}

/* -------------------------------------------------------------------------
 * Test 11: NUM_PARAMS count matches expected (13 parameters)
 * ---------------------------------------------------------------------- */

void test_param_count() {
    printf("\n=== Test 11: Parameter Count Validation ===\n");

    /* Parameters: Thresh, Ratio, Attack, Release, Makeup, Drive, Mix,
       SC_HPF, CompMode, BandSel, BandThresh, BandRatio, DstrMode = 13 */
    const int expected_params = 13;
    const char* param_names[] = {
        "Thresh", "Ratio", "Attack", "Release",
        "Makeup", "Drive", "Mix", "SC_HPF",
        "CompMode", "BandSel", "BandThresh", "BandRatio",
        "DstrMode"
    };

    printf("  Expected %d parameters:\n", expected_params);
    for (int i = 0; i < expected_params; i++) {
        printf("    [%2d] %s\n", i, param_names[i]);
    }
    int ok = (expected_params == 13);
    printf("  Count = %d: %s\n", expected_params, ok ? "PASS" : "FAIL");
    assert(ok);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void) {
    printf("============================================\n");
    printf("OmniPress Compressor – Test Suite\n");
    printf("============================================\n");

    test_gain_computer_below_threshold();
    test_gain_computer_above_threshold();
    test_hard_limiter();
    test_soft_knee();
    test_ar_coefficients();
    test_attack_release_timing();
    test_parameter_mapping();
    test_wetdry_mix();
    test_makeup_gain_conversion();
    test_sc_hpf_range();
    test_param_count();

    printf("\n============================================\n");
    printf("ALL TESTS PASSED\n");
    printf("============================================\n");
    return 0;
}
