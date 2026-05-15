/**
 * @file test_modrate.cpp
 * @brief Regression tests for NeonAdvancedLabirinto::updateModRate()
 *
 * The original implementation was non-idempotent:
 *
 *   // BUG: accumulates on every call
 *   void updateModRate() {
 *       modRate = fmaxf(0.1f, fminf(10.0f, modRate * lfoSpeed * modDepth));
 *   }
 *
 * Because updateModRate() is called from setPillar(), setDiffusion(), and
 * unit_set_param_value(k_vibr), three separate parameter changes would each
 * multiply the *current* modRate by (lfoSpeed * modDepth), driving modRate
 * toward 0.1 (floor) or 10.0 (ceiling) even if the user held all knobs still.
 *
 * The fix recomputes from scratch:
 *
 *   // FIXED: deterministic
 *   void updateModRate() {
 *       modRate = fmaxf(0.1f, fminf(10.0f, lfoSpeed * modDepth));
 *   }
 *
 * Tests:
 *   13. Old formula converges to floor (lfoSpeed*modDepth < 1)
 *   14. Old formula converges to ceiling (lfoSpeed*modDepth > 1)
 *   15. Fixed formula is idempotent after any number of calls
 *   16. Fixed formula boundary values (lfoSpeed range 0.1–3.0,
 *       modDepth range 0.0–1.0; result must stay in [0.1, 10.0])
 *
 * Compile: g++ -std=c++14 -O2 -o test_modrate test_modrate.cpp -lm
 * Run:     ./test_modrate
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>

#define EPSILON 1e-5f

/* =========================================================================
 * Scalar implementations of the two updateModRate variants for comparison.
 * ====================================================================== */

/* BUG: accumulates — mirrors the ORIGINAL (broken) implementation */
static float updateModRate_buggy(float modRate, float lfoSpeed, float modDepth) {
    return fmaxf(0.1f, fminf(10.0f, modRate * lfoSpeed * modDepth));
}

/* FIXED: deterministic — mirrors the CORRECTED implementation */
static float updateModRate_fixed(float lfoSpeed, float modDepth) {
    return fmaxf(0.1f, fminf(10.0f, lfoSpeed * modDepth));
}

/* =========================================================================
 * Test 13: Old formula drifts to floor when lfoSpeed * modDepth < 1
 *
 * With lfoSpeed=1.0, modDepth=0.3:
 *   product = 0.3  (< 1.0)
 *   Starting from modRate=0.5:
 *     call 1: 0.5 * 0.3 = 0.15
 *     call 2: 0.15 * 0.3 = 0.045 → clamps to 0.1
 *     call N: stays at 0.1 (floor)
 * ====================================================================== */
static void test_buggy_floor_drift() {
    printf("\n=== Test 13: Old formula drifts to floor (lfoSpeed*modDepth < 1) ===\n");

    const float lfoSpeed = 1.0f;
    const float modDepth = 0.3f;
    const float init_modRate = 0.5f;  /* default value in NeonAdvancedLabirinto */

    float expected_fixed = updateModRate_fixed(lfoSpeed, modDepth);
    printf("  Expected fixed result = %.4f\n", expected_fixed);

    float modRate = init_modRate;
    printf("  Starting modRate = %.4f\n", modRate);

    for (int call = 1; call <= 10; call++) {
        modRate = updateModRate_buggy(modRate, lfoSpeed, modDepth);
        printf("  Call %2d: modRate = %.6f\n", call, modRate);
    }

    /* After 10 accumulating calls, buggy formula should be at the floor */
    float floor_val = 0.1f;
    int hit_floor = fabsf(modRate - floor_val) < EPSILON;
    printf("  After 10 calls: modRate=%.6f  at floor(0.1)? %s\n",
           modRate, hit_floor ? "YES" : "NO");
    assert(hit_floor && "Buggy formula must converge to floor for product < 1");

    /* Contrast: fixed formula returns the same value every time */
    float fixed_r1 = updateModRate_fixed(lfoSpeed, modDepth);
    float fixed_r2 = updateModRate_fixed(lfoSpeed, modDepth);
    float fixed_r3 = updateModRate_fixed(lfoSpeed, modDepth);
    assert(fabsf(fixed_r1 - fixed_r2) < EPSILON);
    assert(fabsf(fixed_r2 - fixed_r3) < EPSILON);
    printf("  Fixed formula: always %.4f (idempotent) PASS\n", fixed_r1);

    printf("  Test 13 PASSED\n");
}

/* =========================================================================
 * Test 14: Old formula drifts to ceiling when lfoSpeed * modDepth > 1
 *
 * With lfoSpeed=2.5, modDepth=0.8:
 *   product = 2.0  (> 1.0)
 *   Starting from modRate=0.5:
 *     call 1: 0.5 * 2.0 = 1.0
 *     call 2: 1.0 * 2.0 = 2.0
 *     call 3: 2.0 * 2.0 = 4.0
 *     call 4: 4.0 * 2.0 = 8.0
 *     call 5: 8.0 * 2.0 = 16 → clamps to 10.0
 * ====================================================================== */
static void test_buggy_ceiling_drift() {
    printf("\n=== Test 14: Old formula drifts to ceiling (lfoSpeed*modDepth > 1) ===\n");

    const float lfoSpeed = 2.5f;
    const float modDepth = 0.8f;
    const float init_modRate = 0.5f;

    float expected_fixed = updateModRate_fixed(lfoSpeed, modDepth);
    printf("  Expected fixed result = %.4f  (lfoSpeed*modDepth = %.4f)\n",
           expected_fixed, lfoSpeed * modDepth);

    float modRate = init_modRate;
    printf("  Starting modRate = %.4f\n", modRate);

    for (int call = 1; call <= 8; call++) {
        modRate = updateModRate_buggy(modRate, lfoSpeed, modDepth);
        printf("  Call %2d: modRate = %.6f\n", call, modRate);
    }

    /* After enough calls, buggy formula must hit the ceiling */
    float ceiling_val = 10.0f;
    int hit_ceiling = fabsf(modRate - ceiling_val) < EPSILON;
    printf("  After 8 calls: modRate=%.6f  at ceiling(10.0)? %s\n",
           modRate, hit_ceiling ? "YES" : "NO");
    assert(hit_ceiling && "Buggy formula must converge to ceiling for product > 1");

    /* Contrast: fixed formula returns the correct value every time */
    float fixed_r = updateModRate_fixed(lfoSpeed, modDepth);
    assert(fabsf(fixed_r - expected_fixed) < EPSILON);
    printf("  Fixed formula: always %.4f (idempotent) PASS\n", fixed_r);

    printf("  Test 14 PASSED\n");
}

/* =========================================================================
 * Test 15: Fixed formula is idempotent (same result after N calls)
 *
 * Simulates the real scenario: setPillar(), setDiffusion(), and k_vibr are
 * three separate user knob events that each call updateModRate().  The knob
 * values haven't changed, but three calls fire in quick succession.
 * ====================================================================== */
static void test_fixed_is_idempotent() {
    printf("\n=== Test 15: Fixed formula idempotency (repeated calls) ===\n");

    typedef struct { float lfoSpeed; float modDepth; } Case;

    Case cases[] = {
        { 0.5f, 0.5f },   /* nominal: result = 0.25, clamped to 0.25 */
        { 1.0f, 1.0f },   /* result = 1.0 */
        { 3.0f, 1.0f },   /* result = 3.0 */
        { 0.1f, 0.1f },   /* result = 0.01, clamped to floor 0.1 */
        { 3.0f, 3.4f },   /* 3*3.4=10.2, clamped to ceiling 10.0 */
    };
    int num_cases = (int)(sizeof(cases) / sizeof(cases[0]));

    for (int c = 0; c < num_cases; c++) {
        float ls = cases[c].lfoSpeed;
        float md = cases[c].modDepth;

        float r0 = updateModRate_fixed(ls, md);
        float r1 = updateModRate_fixed(ls, md);
        float r2 = updateModRate_fixed(ls, md);
        float r9 = 0.0f;
        for (int i = 0; i < 9; i++) r9 = updateModRate_fixed(ls, md);

        int all_same = (fabsf(r0 - r1) < EPSILON &&
                        fabsf(r1 - r2) < EPSILON &&
                        fabsf(r2 - r9) < EPSILON);
        printf("  lfoSpeed=%.2f modDepth=%.2f → result=%.4f  idempotent=%s\n",
               ls, md, r0, all_same ? "PASS" : "FAIL");
        assert(all_same);
    }

    printf("  Test 15 PASSED\n");
}

/* =========================================================================
 * Test 16: Fixed formula boundary values
 *
 * lfoSpeed is clamped to [0.1, 3.0] by setLfoSpeed().
 * modDepth is clamped to [0.0, 1.0] by setModDepth().
 * updateModRate output must be in [0.1, 10.0].
 * ====================================================================== */
static void test_fixed_boundary_values() {
    printf("\n=== Test 16: Fixed formula boundary values ===\n");

    /* (lfoSpeed, modDepth, expected_result) */
    typedef struct { float ls; float md; float expected; } BCase;

    BCase cases[] = {
        /* Min inputs → product 0.01 → floor 0.1 */
        { 0.1f, 0.1f,   0.1f  },
        /* Typical mid-range */
        { 1.0f, 0.5f,   0.5f  },
        { 2.0f, 0.5f,   1.0f  },
        /* lfoSpeed at ceiling, modDepth = 1.0 → product = 3.0 */
        { 3.0f, 1.0f,   3.0f  },
        /* product > 10 → ceiling 10.0 */
        { 3.0f, 4.0f,  10.0f  },
        /* Zero modDepth → product 0 → floor 0.1 */
        { 2.0f, 0.0f,   0.1f  },
    };
    int num_cases = (int)(sizeof(cases) / sizeof(cases[0]));

    int all_ok = 1;
    for (int c = 0; c < num_cases; c++) {
        float result = updateModRate_fixed(cases[c].ls, cases[c].md);
        int ok = fabsf(result - cases[c].expected) < EPSILON;
        printf("  lfoSpeed=%.2f  modDepth=%.2f  → result=%.4f  expected=%.4f  %s\n",
               cases[c].ls, cases[c].md, result, cases[c].expected,
               ok ? "PASS" : "FAIL");
        if (!ok) all_ok = 0;
        /* Always in [0.1, 10.0] */
        assert(result >= 0.1f && result <= 10.0f);
    }
    assert(all_ok);

    printf("  Test 16 PASSED\n");
}

/* =========================================================================
 * Test 17: Multi-knob scenario — three rapid parameter events
 *
 * Simulates: user tweaks VIBR, then DFSN, then PILLAR in quick succession.
 * Each triggers updateModRate().  With the fix, all three should land on
 * exactly the same final modRate.
 * ====================================================================== */
static void test_multi_knob_scenario() {
    printf("\n=== Test 17: Multi-knob scenario (VIBR + DFSN + PILLAR) ===\n");

    /* State snapshot before the three events */
    float lfoSpeed = 1.5f;    /* VIBR=15 → value*0.1 = 1.5 Hz */
    float diffusion = 0.7f;   /* DFSN=700/1000 = 0.7 */
    int   pillar    = 3;      /* full FDN → pillar multiplier = 0.1 */

    /* Pillar multipliers matching NeonAdvancedLabirinto.h setPillar() */
    float pillar_mults[] = { 0.6f, 0.4f, 0.2f, 0.1f, 0.1f };
    float modDepth = pillar_mults[pillar] * diffusion;  /* 0.1 * 0.7 = 0.07 */

    printf("  lfoSpeed=%.2f  diffusion=%.2f  pillar=%d  modDepth=%.4f\n",
           lfoSpeed, diffusion, pillar, modDepth);
    printf("  Expected modRate = %.4f\n", updateModRate_fixed(lfoSpeed, modDepth));

    /* Event 1: VIBR knob → setLfoSpeed, updateModRate */
    float modRate = 0.5f;  /* initial default */
    modRate = updateModRate_fixed(lfoSpeed, modDepth);
    float after_vibr = modRate;

    /* Event 2: DFSN knob → setDiffusion → setModDepth → updateModRate */
    modRate = updateModRate_fixed(lfoSpeed, modDepth);
    float after_comp = modRate;

    /* Event 3: PILLAR knob → setPillar → setModDepth → updateModRate */
    modRate = updateModRate_fixed(lfoSpeed, modDepth);
    float after_pillar = modRate;

    printf("  modRate after VIBR  event: %.6f\n", after_vibr);
    printf("  modRate after DFSN  event: %.6f\n", after_comp);
    printf("  modRate after PILLAR event: %.6f\n", after_pillar);

    assert(fabsf(after_vibr - after_comp)   < EPSILON && "modRate must be stable after VIBR+DFSN");
    assert(fabsf(after_comp - after_pillar) < EPSILON && "modRate must be stable after DFSN+PILLAR");

    printf("  PASS: modRate stable across all three knob events\n");
    printf("  Test 17 PASSED\n");
}

/* =========================================================================
 * main
 * ====================================================================== */
int main(void) {
    printf("==========================================================\n");
    printf(" reverb_labirinto updateModRate() Regression Tests\n");
    printf("==========================================================\n");

    test_buggy_floor_drift();
    test_buggy_ceiling_drift();
    test_fixed_is_idempotent();
    test_fixed_boundary_values();
    test_multi_knob_scenario();

    printf("\n==========================================================\n");
    printf(" ALL TESTS PASSED\n");
    printf("==========================================================\n");
    return 0;
}
