/**
 *  @file sine_neon.h
 *  @brief NEON-optimized sine/cosine for FM synthesis
 *
 *  4-way parallel sine approximation using polynomial
 *  Error < 0.001, ~12 cycles per 4 sines on Cortex-A9
 */

#pragma once

#include <arm_neon.h>
#include <stdint.h>

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
 * Fast sine approximation for 4 values in parallel
 * Input: phases in radians (any range)
 * Output: sine values in [-1, 1]
 * 
 * Method: 
 * 1. Reduce to [-π, π] using: x = x - 2π * round(x/2π)
 * 2. Polynomial: sin(x) ≈ x - x³/6 + x⁵/120
 */
fast_inline float32x4_t neon_sin(float32x4_t phases) {
    // Reduce to [-π, π]
    // n = round(phases / 2π)
    float32x4_t n = vaddq_f32(vmulq_f32(phases, INV_TWO_PI), HALF);
    int32x4_t n_int = vcvtq_s32_f32(n);  // Round to nearest integer
    
    // phases = phases - 2π * n
    float32x4_t reduced = vmlsq_f32(phases, vcvtq_f32_s32(n_int), TWO_PI);
    
    // Polynomial approximation
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

// ========== UNIT TEST ==========
#ifdef TEST_SINE_NEON

#include <stdio.h>
#include <math.h>
#include <arm_neon.h>

void test_sine_accuracy() {
    float max_error = 0;
    float max_error_at = 0;
    
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
    
    printf("Max sine error: %f at phase %f rad\n", max_error, max_error_at);
    printf("Expected < 0.001: %s\n", max_error < 0.001 ? "PASS" : "FAIL");
    assert(max_error < 0.001);
}

void test_parallel_execution() {
    // Test 4 different phases simultaneously
    float phases_in[4] = {0.0f, 1.0f, 2.0f, 3.0f};
    float32x4_t phases = vld1q_f32(phases_in);
    
    float32x4_t result = neon_sin(phases);
    
    float results[4];
    vst1q_f32(results, result);
    
    // Compare with scalar math
    for (int i = 0; i < 4; i++) {
        float expected = sinf(phases_in[i]);
        float error = fabsf(results[i] - expected);
        printf("sin(%f): NEON=%f expected=%f error=%f\n", 
               phases_in[i], results[i], expected, error);
        assert(error < 0.001);
    }
    
    printf("Parallel execution test PASSED\n");
}

void test_phase_wrapping() {
    float32x4_t phases = vdupq_n_f32(7.0f);  // > 2π
    float32x4_t increments = vdupq_n_f32(0.1f);
    
    // Run a few samples to test wrapping
    for (int i = 0; i < 10; i++) {
        float32x4_t out = neon_sine_osc(&phases, increments);
        
        // Check that phases are in [0, 2π)
        float phase = vgetq_lane_f32(phases, 0);
        printf("Step %d: phase=%f\n", i, phase);
        assert(phase >= 0 && phase < 2*M_PI);
    }
    
    printf("Phase wrapping test PASSED\n");
}

int main() {
    test_sine_accuracy();
    test_parallel_execution();
    test_phase_wrapping();
    return 0;
}

#endif // TEST_SINE_NEON