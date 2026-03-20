/**
 * @file benchmark_resonant.h
 * @brief Performance benchmark for resonant synthesis
 *
 * Measures CPU cycles for division operations
 */

#include <arm_neon.h>
#include <stdio.h>
#include <stdint.h>

// Read CPU cycle counter (ARM Cortex-A9)
static inline uint32_t read_cycle_counter(void) {
    uint32_t cycles;
    asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (cycles));
    return cycles;
}

void benchmark_resonant_operations() {
    const int ITERATIONS = 10000;

    float32x4_t a = vdupq_n_f32(0.5f);
    float32x4_t b = vdupq_n_f32(1.5f);
    float32x4_t c = vdupq_n_f32(2.0f);
    float32x4_t d = vdupq_n_f32(3.0f);
    float32x4_t result;

    // Benchmark 1: Standard division (slow)
    uint32_t start = read_cycle_counter();
    for (int i = 0; i < ITERATIONS; i++) {
        result = fast_div_neon(a, b);
    }
    uint32_t end = read_cycle_counter();
    uint32_t div_cycles = (end - start) / ITERATIONS;
    printf("fast_div_neon: %d cycles\n", div_cycles);

    // Benchmark 2: Division using reciprocal + multiply (faster)
    start = read_cycle_counter();
    for (int i = 0; i < ITERATIONS; i++) {
        float32x4_t recip = vrecpeq_f32(b);
        recip = vmulq_f32(vrecpsq_f32(b, recip), recip);
        result = vmulq_f32(a, recip);
    }
    end = read_cycle_counter();
    uint32_t recip_cycles = (end - start) / ITERATIONS;
    printf("reciprocal method: %d cycles\n", recip_cycles);

    // Benchmark 3: Approximate division (for denom protection)
    start = read_cycle_counter();
    for (int i = 0; i < ITERATIONS; i++) {
        // Denom protection using vmaxq_f32 is cheap
        float32x4_t denom = vmaxq_f32(b, vdupq_n_f32(0.0001f));
        float32x4_t recip = vrecpeq_f32(denom);
        recip = vmulq_f32(vrecpsq_f32(denom, recip), recip);
        result = vmulq_f32(a, recip);
    }
    end = read_cycle_counter();
    uint32_t protected_cycles = (end - start) / ITERATIONS;
    printf("protected division: %d cycles\n", protected_cycles);

    // Expected results:
    // - fast_div_neon: ~20-30 cycles
    // - reciprocal method: ~8-12 cycles
    // - protected division: ~10-14 cycles
}