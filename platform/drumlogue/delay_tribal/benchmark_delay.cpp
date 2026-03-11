/**
 * @file benchmark_delay.cpp
 * @brief Performance benchmark for Percussion Spatializer
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "PercussionSpatializer.h"

// ARM cycle counter
static inline uint32_t read_cycle_counter(void) {
    uint32_t cycles;
    asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r" (cycles));
    return cycles;
}

void benchmark_delay_line_access() {
    printf("\n=== Benchmark: Delay Line Access ===\n");

    const int ITERATIONS = 100000;
    PercussionSpatializer spatializer;

    // Initialize with dummy data
    unit_runtime_desc_t desc;
    desc.samplerate = 48000;
    desc.input_channels = 2;
    desc.output_channels = 2;
    spatializer.Init(&desc);

    // Benchmark gather operation
    uint32_t start = read_cycle_counter();
    for (int i = 0; i < ITERATIONS; i++) {
        // Force gather operation
        float in[8] = {0};
        float out[8];
        spatializer.Process(in, out, 1);
    }
    uint32_t end = read_cycle_counter();

    printf("Cycles per sample: %d\n", (end - start) / ITERATIONS);
}

void benchmark_lfo_generation() {
    printf("\n=== Benchmark: LFO Generation ===\n");

    // Compare table lookup vs calculation
    const int ITERATIONS = 100000;

    // Method 1: Calculation
    uint32_t start = read_cycle_counter();
    float sum1 = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        float phase = (float)i / ITERATIONS;
        sum1 += 2.0f * fabsf(phase - 0.5f);
    }
    uint32_t end = read_cycle_counter();
    uint32_t calc_cycles = (end - start) / ITERATIONS;

    // Method 2: Table lookup
    float table[256];
    for (int i = 0; i < 256; i++) {
        float phase = (float)i / 256.0f;
        table[i] = 2.0f * fabsf(phase - 0.5f);
    }

    start = read_cycle_counter();
    float sum2 = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i & 255;
        sum2 += table[idx];
    }
    end = read_cycle_counter();
    uint32_t table_cycles = (end - start) / ITERATIONS;

    printf("Calculation: %d cycles\n", calc_cycles);
    printf("Table lookup: %d cycles\n", table_cycles);
    printf("Speedup: %.1fx\n", (float)calc_cycles / table_cycles);
}

int main() {
    printf("========================================\n");
    printf("Percussion Spatializer - Phase 5 Benchmarks\n");
    printf("========================================\n");

    benchmark_delay_line_access();
    benchmark_lfo_generation();

    return 0;
}
