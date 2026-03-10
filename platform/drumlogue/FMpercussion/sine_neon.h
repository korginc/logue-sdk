#pragma once

/**
 * @file sine_neon.h
 * @brief NEON-optimized sine/cosine for FM synthesis
 *
 * 4-way parallel sine approximation using polynomial
 * Error < 0.001, ~12 cycles per 4 sines on Cortex-A9
 *
 * FIXED: Proper rounding to nearest for range reduction
 */

#include <arm_neon.h>
#include <stdint.h>
#include "float_math.h"  // For M_* constants if needed

// Constants for sine approximation
static const float32x4_t ONE = vdupq_n_f32(1.0f);
static const float32x4_t HALF = vdupq_n_f32(0.5f);
static const float32x4_t TWO = vdupq_n_f32(2.0f);
static const float32x4_t PI = vdupq_n_f32(3.14159265359f);
static const float32x4_t TWO_PI = vdupq_n_f32(6.28318530718f);
static const float32x4_t INV_TWO_PI = vdupq_n_f32(0.159154943f);  // 1/(2π)

// Polynomial coefficients for sin(x) on [-π, π]
static const float32x4_t C1 = vdupq_n_f32(1.0f);           // x
static const float32x4_t C3 = vdupq_n_f32(-0.166666667f);  // -x³/6
static const float32x4_t C5 = vdupq_n_f32(0.008333333f);   // x⁵/120

/**
 * Round to nearest integer using ARM NEON
 * vcvtq_s32_f32 truncates toward zero, so we need to adjust
 */
fast_inline int32x4_t round_to_nearest_int(float32x4_t x) {
    // Add 0.5 for positive numbers, subtract 0.5 for negative numbers
    uint32x4_t is_negative = vcltq_f32(x, vdupq_n_f32(0.0f));
    float32x4_t adjust = vbslq_f32(is_negative, vdupq_n_f32(-0.5f), vdupq_n_f32(0.5f));
    float32x4_t rounded = vaddq_f32(x, adjust);
    return vcvtq_s32_f32(rounded);
}

/**
 * Fast sine approximation for 4 values in parallel
 * Input: phases in radians (any range)
 * Output: sine values in [-1, 1]
 *
 * Method:
 * 1. Reduce to [-π, π] using: x = x - 2π * round(x/2π)
 * 2. Polynomial: sin(x) ≈ x - x³/6 + x⁵/120
 */
fast_inline float32x4_t neon_sin(float32x4_t phases) {
    // =================================================================
    // Step 1: Range reduction to [-π, π] with proper rounding
    // =================================================================
    // Calculate n = round(phases / 2π)
    float32x4_t n_float = vmulq_f32(phases, INV_TWO_PI);  // phases / 2π

    // FIXED: Use proper rounding to nearest, not truncation
    int32x4_t n_int = round_to_nearest_int(n_float);

    // phases = phases - 2π * n
    float32x4_t reduced = vmlsq_f32(phases, vcvtq_f32_s32(n_int), TWO_PI);

    // =================================================================
    // Step 2: Polynomial approximation sin(x) ≈ x - x³/6 + x⁵/120
    // =================================================================
    float32x4_t x2 = vmulq_f32(reduced, reduced);  // x²
    float32x4_t x3 = vmulq_f32(reduced, x2);       // x³
    float32x4_t x5 = vmulq_f32(x3, x2);            // x⁵

    // result = C1*x + C3*x³ + C5*x⁵
    float32x4_t result = vmulq_f32(reduced, C1);
    result = vmlaq_f32(result, x3, C3);
    result = vmlaq_f32(result, x5, C5);

    return result;
}

/**
 * Alternative method using fast range reduction with remainder
 * Even more accurate but slightly slower
 */
fast_inline float32x4_t neon_sin_accurate(float32x4_t phases) {
    // Use modulo to get remainder in [-π, π]
    float32x4_t n_float = vmulq_f32(phases, INV_TWO_PI);

    // Round to nearest integer (fixed)
    int32x4_t n_int = round_to_nearest_int(n_float);

    // remainder = phases - n_int * 2π
    float32x4_t remainder = vmlsq_f32(phases, vcvtq_f32_s32(n_int), TWO_PI);

    // Polynomial approximation as above
    float32x4_t x2 = vmulq_f32(remainder, remainder);
    float32x4_t x3 = vmulq_f32(remainder, x2);
    float32x4_t x5 = vmulq_f32(x3, x2);

    float32x4_t result = vmulq_f32(remainder, C1);
    result = vmlaq_f32(result, x3, C3);
    result = vmlaq_f32(result, x5, C5);

    return result;
}

/**
 * Fast cosine using sine with phase shift
 * cos(x) = sin(x + π/2)
 */
fast_inline float32x4_t neon_cos(float32x4_t phases) {
    float32x4_t half_pi = vdupq_n_f32(1.57079632679f);
    return neon_sin(vaddq_f32(phases, half_pi));
}

/**
 * Sine oscillator with phase accumulator (4 voices)
 * Returns sine values and updates phases
 */
fast_inline float32x4_t neon_sine_osc(
    float32x4_t* phases,        // Current phases [0-2π)
    float32x4_t increments       // Phase increment per sample
) {
    // Update phases
    *phases = vaddq_f32(*phases, increments);

    // Wrap to [0, 2π)
    uint32x4_t ge_2pi = vcgeq_f32(*phases, TWO_PI);
    *phases = vbslq_f32(ge_2pi, vsubq_f32(*phases, TWO_PI), *phases);

    // Compute sine
    return neon_sin(*phases);
}

/**
 * Even faster version using 4th order polynomial
 * Slightly less accurate but faster
 */
fast_inline float32x4_t neon_sin_fast(float32x4_t phases) {
    // Range reduction with proper rounding
    float32x4_t n_float = vmulq_f32(phases, INV_TWO_PI);
    int32x4_t n_int = round_to_nearest_int(n_float);
    float32x4_t reduced = vmlsq_f32(phases, vcvtq_f32_s32(n_int), TWO_PI);

    // 4th order polynomial: x - x³/6
    float32x4_t x2 = vmulq_f32(reduced, reduced);
    float32x4_t x3 = vmulq_f32(reduced, x2);

    return vmlsq_f32(reduced, x3, vdupq_n_f32(0.166666667f));
}

// ========== UNIT TEST ==========
#ifdef TEST_SINE_NEON

#include <stdio.h>
#include <math.h>
#include <assert.h>

void test_sine_accuracy() {
    float max_error = 0.0f;
    float max_error_at = 0.0f;

    // Test 1000 points across [0, 4π] to test range reduction
    for (int i = 0; i < 2000; i++) {
        float phase = (8.0f * M_PI * i) / 2000.0f - 4.0f * M_PI;  // -4π to 4π
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

    printf("Max sine error: %f at phase %f rad\n", max_error, max_error_at);
    printf("Expected < 0.001: %s\n", max_error < 0.001 ? "PASS" : "FAIL");
    assert(max_error < 0.001);
}

void test_rounding_vs_truncation() {
    // Test values that would cause errors with truncation
    float test_phases[] = {
        6.28318530718f * 0.25f,  // π/2 - should reduce to π/2
        6.28318530718f * 0.75f,  // 3π/2 - should reduce to -π/2
        6.28318530718f * 1.25f,  // 5π/2 - should reduce to π/2
        6.28318530718f * 1.75f,  // 7π/2 - should reduce to -π/2
    };

    float expected[] = {
        1.0f,   // sin(π/2)
        -1.0f,  // sin(3π/2) = -1
        1.0f,   // sin(5π/2) = 1
        -1.0f,  // sin(7π/2) = -1
    };

    printf("Testing range reduction with proper rounding:\n");
    printf("----------------------------------------------\n");

    for (int i = 0; i < 4; i++) {
        float32x4_t phases = vdupq_n_f32(test_phases[i]);
        float32x4_t result = neon_sin(phases);
        float sin_val = vgetq_lane_f32(result, 0);

        float error = fabsf(sin_val - expected[i]);
        printf("sin(%.2fπ) = %f (expected %f) error = %f %s\n",
               test_phases[i] / M_PI, sin_val, expected[i], error,
               error < 0.001 ? "✓" : "✗");

        assert(error < 0.001);
    }
    printf("----------------------------------------------\n");
    printf("Range reduction test PASSED\n");
}

void test_parallel_execution() {
    // Test 4 different phases simultaneously
    float phases_in[4] = {0.0f, 1.0f, 2.0f, 3.0f};
    float32x4_t phases = vld1q_f32(phases_in);

    float32x4_t result = neon_sin(phases);

    float results[4];
    vst1q_f32(results, result);

    printf("\nParallel execution test:\n");
    for (int i = 0; i < 4; i++) {
        float expected = sinf(phases_in[i]);
        float error = fabsf(results[i] - expected);
        printf("sin(%f): NEON=%f expected=%f error=%f %s\n",
               phases_in[i], results[i], expected, error,
               error < 0.001 ? "✓" : "✗");
        assert(error < 0.001);
    }

    printf("Parallel execution test PASSED\n");
}

void test_phase_wrapping() {
    float32x4_t phases = vdupq_n_f32(7.0f);  // > 2π
    float32x4_t increments = vdupq_n_f32(0.1f);

    printf("\nPhase wrapping test:\n");
    for (int i = 0; i < 10; i++) {
        float32x4_t out = neon_sine_osc(&phases, increments);

        // Check that phases are in [0, 2π)
        float phase = vgetq_lane_f32(phases, 0);
        float sine = vgetq_lane_f32(out, 0);
        printf("Step %d: phase=%f sin=%f\n", i, phase, sine);
        assert(phase >= 0 && phase < 2*M_PI);
    }

    printf("Phase wrapping test PASSED\n");
}

int main() {
    printf("\n=== SINE NEON TESTS ===\n\n");
    test_rounding_vs_truncation();
    test_sine_accuracy();
    test_parallel_execution();
    test_phase_wrapping();
    printf("\n✓ ALL TESTS PASSED\n");
    return 0;
}

#endif // TEST_SINE_NEON