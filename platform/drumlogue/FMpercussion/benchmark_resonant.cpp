/**
 * @file benchmark_resonant.cpp
 * @brief CPU cycle benchmarks for resonant synthesis
 * 
 * Compile: g++ -o bench benchmark_resonant.cpp -O3 -mfpu=neon
 * Run: ./bench
 */

#include <arm_neon.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

// Read CPU cycle counter (ARM Cortex-A9)
static inline uint32_t read_cycle_counter(void) {
    uint32_t cycles;
    asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (cycles));
    return cycles;
}

// Simple resonant process simulation
float32x4_t resonant_process_sim(float32x4_t sin_f0, float32x4_t cos_fc,
                                  float32x4_t a, float32x4_t one) {
    float32x4_t a_sq = vmulq_f32(a, a);
    float32x4_t two_a = vmulq_f32(vdupq_n_f32(2.0f), a);
    
    float32x4_t denom = vsubq_f32(vaddq_f32(one, a_sq),
                                   vmulq_f32(two_a, cos_fc));
    
    // Protect against division by zero
    denom = vmaxq_f32(denom, vdupq_n_f32(0.0001f));
    
    // Division - the expensive part
    float32x4_t result = vdivq_f32(sin_f0, denom);
    
    return result;
}

// Optimized version using reciprocal
float32x4_t resonant_process_opt(float32x4_t sin_f0, float32x4_t cos_fc,
                                  float32x4_t a, float32x4_t one) {
    float32x4_t a_sq = vmulq_f32(a, a);
    float32x4_t two_a = vmulq_f32(vdupq_n_f32(2.0f), a);
    
    float32x4_t denom = vsubq_f32(vaddq_f32(one, a_sq),
                                   vmulq_f32(two_a, cos_fc));
    
    // Protect against division by zero
    denom = vmaxq_f32(denom, vdupq_n_f32(0.0001f));
    
    // Use reciprocal + multiply instead of division
    float32x4_t recip = vrecpeq_f32(denom);
    // One Newton-Raphson iteration for better accuracy
    recip = vmulq_f32(vrecpsq_f32(denom, recip), recip);
    
    float32x4_t result = vmulq_f32(sin_f0, recip);
    
    return result;
}

void benchmark_division() {
    const int ITERATIONS = 100000;
    
    float32x4_t sin_f0 = vdupq_n_f32(0.5f);
    float32x4_t cos_fc = vdupq_n_f32(0.5f);
    float32x4_t a = vdupq_n_f32(0.5f);
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t result;
    
    printf("\n=== Resonant Synthesis Benchmarks ===\n");
    
    // Benchmark 1: Direct division
    uint32_t start = read_cycle_counter();
    for (int i = 0; i < ITERATIONS; i++) {
       