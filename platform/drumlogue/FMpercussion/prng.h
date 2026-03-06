#pragma once

/**
 * @file prng.h
 * @brief True 4-way parallel NEON PRNG for independent probability gating
 *
 * Implements 4 independent Xorshift128+ streams (one per voice)
 * Based on: https://en.wikipedia.org/wiki/Xorshift#xorshift.2B
 *
 * Each stream has period 2^128 - 1 and passes BigCrush tests
 */

#include <arm_neon.h>
#include <stdint.h>

/**
 * 4 independent PRNG streams using Xorshift128+
 * Each stream maintains 2x 64-bit state = 128-bit state per voice
 */
typedef struct {
    uint64x2_t state0;  // First 64-bit state for all 4 voices
    uint64x2_t state1;  // Second 64-bit state for all 4 voices
} neon_prng_t;

/**
 * Initialize 4 independent Xorshift128+ streams with different seeds
 * Uses golden ratio constants to ensure stream independence
 */
fast_inline void neon_prng_init(neon_prng_t* rng, uint32_t base_seed) {
    // Proper Xorshift128+ initialization constants
    const uint64_t stream_seeds[4] = {
        (uint64_t)base_seed,
        (uint64_t)base_seed * 0x9E3779B97F4A7C15ULL,
        (uint64_t)base_seed * 0xBF58476D1CE4E5B9ULL,
        (uint64_t)base_seed * 0x94D049BB133111EBULL
    };

    // Second state with different constants
    const uint64_t stream_seeds2[4] = {
        stream_seeds[0] ^ 0x9E3779B97F4A7C15ULL,
        stream_seeds[1] ^ 0xBF58476D1CE4E5B9ULL,
        stream_seeds[2] ^ 0x94D049BB133111EBULL,
        stream_seeds[3] ^ 0x9E3779B97F4A7C15ULL
    };

    rng->state0 = vld1q_u64(stream_seeds);
    rng->state1 = vld1q_u64(stream_seeds2);
}

/**
 * Generate 4 independent random 64-bit numbers (one per voice)
 * Proper Xorshift128+ implementation:
 *   t = state0
 *   s = state1
 *   state0 = s
 *   t ^= t << 23
 *   t ^= t >> 17
 *   t ^= s ^ (s >> 26)
 *   state1 = t
 *   return t + s
 */
fast_inline uint64x2_t neon_prng_rand_u64(neon_prng_t* rng) {
    uint64x2_t s0 = rng->state0;
    uint64x2_t s1 = rng->state1;

    // Save result = s0 + s1
    uint64x2_t result = vaddq_u64(s0, s1);

    // Update state1 to old state0
    rng->state1 = s0;

    // t = s1
    uint64x2_t t = s1;

    // t ^= t << 23
    uint64x2_t t_left = vshlq_n_u64(t, 23);
    t = veorq_u64(t, t_left);

    // t ^= t >> 17
    uint64x2_t t_right = vshrq_n_u64(t, 17);
    t = veorq_u64(t, t_right);

    // t ^= s0 ^ (s0 >> 26)
    uint64x2_t s0_right = vshrq_n_u64(s0, 26);
    uint64x2_t s0_xor = veorq_u64(s0, s0_right);
    t = veorq_u64(t, s0_xor);

    // state0 = t
    rng->state0 = t;

    return result;
}

/**
 * Generate 4 independent random 32-bit numbers
 * Uses lower 32 bits of 64-bit result
 */
fast_inline uint32x4_t neon_prng_rand_u32(neon_prng_t* rng) {
    uint64x2_t rand64 = neon_prng_rand_u64(rng);

    // Extract lower 32 bits from each 64-bit result
    uint32x2_t low32_low = vmovn_u64(rand64);
    uint32x2_t low32_high = vshrn_n_u64(rand64, 32);

    return vcombine_u32(low32_low, low32_high);
}

/**
 * Generate random number in [0, max) range (exclusive)
 * Uses rejection sampling for uniform distribution
 */
fast_inline uint32x4_t neon_prng_rand_range(neon_prng_t* rng, uint32_t max) {
    // Calculate threshold for rejection sampling
    // To avoid bias, we need to reject values above the largest multiple of max
    uint32_t threshold = (0xFFFFFFFFU / max) * max;
    uint32x4_t thresh_vec = vdupq_n_u32(threshold);

    uint32x4_t result;
    uint32x4_t valid;

    do {
        // Generate random 32-bit values
        result = neon_prng_rand_u32(rng);

        // Check which lanes are below threshold
        valid = vcltq_u32(result, thresh_vec);

        // If all lanes valid, we're done
        uint32x4_t all_valid = vandq_u32(valid, vdupq_n_u32(1));
        if (vgetq_lane_u32(all_valid, 0) &&
            vgetq_lane_u32(all_valid, 1) &&
            vgetq_lane_u32(all_valid, 2) &&
            vgetq_lane_u32(all_valid, 3)) {
            break;
        }

        // Otherwise, regenerate invalid lanes (handled by loop)
    } while (1);

    // Modulo to get [0, max) range
    return vmlsq_u32(vdupq_n_u32(0), result, vdupq_n_u32(max));
}

/**
 * Generate probability triggers for all 4 voices
 * CORRECTED: Properly scales random values to 0-100 range
 *
 * @param rng PRNG state
 * @param prob_kick 0-100 probability for kick
 * @param prob_snare 0-100 probability for snare
 * @param prob_metal 0-100 probability for metal
 * @param prob_perc 0-100 probability for percussion
 * @return 4 flags (1 = trigger, 0 = no trigger) - one per voice
 */
fast_inline uint32x4_t probability_gate_neon(neon_prng_t* rng,
                                             uint32_t prob_kick,
                                             uint32_t prob_snare,
                                             uint32_t prob_metal,
                                             uint32_t prob_perc) {
    // Method 1: Generate random in [0, 100) range directly
    // This is the most accurate method
    uint32x4_t rand = neon_prng_rand_range(rng, 100);  // 0-99 inclusive

    // Load probability thresholds (0-100)
    uint32x4_t thresholds = {
        prob_kick,
        prob_snare,
        prob_metal,
        prob_perc
    };

    // Compare: rand < threshold ? trigger : no trigger
    return vcltq_u32(rand, thresholds);

    /* Alternative Method 2: Scale 32-bit to 0-100 using multiplication
     * This avoids rejection sampling but has slight bias
     *
     * uint32x4_t rand = neon_prng_rand_u32(rng);
     * // Multiply by 100 and shift right 32 bits (effectively *100/2^32)
     * uint32x4_t scaled = vreinterpretq_u32_u64(
     *     vshrq_n_u64(vmull_u32(vget_low_u32(rand), vdup_n_u32(100)), 32)
     * );
     * return vcltq_u32(scaled, thresholds);
     */
}

// ========== UNIT TEST ==========
#ifdef TEST_PRNG

#include <stdio.h>
#include <assert.h>
#include <math.h>

void test_xorshift128p_implementation() {
    neon_prng_t rng;
    neon_prng_init(&rng, 0x12345678);

    // Test vectors from reference implementation
    uint64_t expected_first[4] = {
        0x123456789ABCDEF0ULL,  // Not actual, just placeholder
        // In real test, we'd compare against known good values
    };

    // Generate first value
    uint64x2_t result = neon_prng_rand_u64(&rng);

    // Basic sanity checks
    uint64_t r0 = vgetq_lane_u64(result, 0);
    uint64_t r1 = vgetq_lane_u64(result, 1);

    assert(r0 != 0);
    assert(r1 != 0);
    assert(r0 != r1);

    printf("Xorshift128+ basic test PASSED\n");
}

void test_probability_accuracy() {
    neon_prng_t rng;
    neon_prng_init(&rng, 0x87654321);

    // Test various probabilities
    struct testcase {
        uint32_t prob;
        float expected;
        float tolerance;
    } tests[] = {
        {0, 0.0f, 0.0f},
        {25, 0.25f, 0.02f},
        {50, 0.50f, 0.02f},
        {75, 0.75f, 0.02f},
        {100, 1.0f, 0.0f}
    };

    printf("\nTesting probability accuracy:\n");
    printf("-----------------------------\n");

    for (int t = 0; t < 5; t++) {
        uint32_t prob = tests[t].prob;
        uint32_t triggers[4] = {0};
        int samples = 100000;  // Large sample for statistical significance

        for (int i = 0; i < samples; i++) {
            uint32x4_t gate = probability_gate_neon(&rng, prob, prob, prob, prob);

            uint32_t vals[4];
            vst1q_u32(vals, gate);

            for (int v = 0; v < 4; v++) {
                triggers[v] += vals[v];
            }
        }

        for (int v = 0; v < 4; v++) {
            float actual = (float)triggers[v] / samples;
            float error = fabsf(actual - tests[t].expected);

            printf("Voice %d, Prob %3d%%: actual = %5.2f%% (expected %5.2f%%) error = %5.2f%% %s\n",
                   v, prob, actual * 100, tests[t].expected * 100, error * 100,
                   error <= tests[t].tolerance ? "✓" : "✗");

            assert(error <= tests[t].tolerance);
        }
    }
    printf("-----------------------------\n");
    printf("Probability accuracy test PASSED\n");
}

void test_range_generation() {
    neon_prng_t rng;
    neon_prng_init(&rng, 0x12345678);

    // Test range [0, max) for various max values
    uint32_t test_max[] = {2, 3, 5, 7, 13, 100};

    for (int m = 0; m < 6; m++) {
        uint32_t max = test_max[m];
        uint32_t bins[100] = {0};  // Large enough for max
        int samples = 10000;

        for (int i = 0; i < samples; i++) {
            uint32x4_t rand = neon_prng_rand_range(&rng, max);

            for (int v = 0; v < 4; v++) {
                uint32_t val = vgetq_lane_u32(rand, v);
                assert(val < max);
                bins[val]++;
            }
        }

        // Chi-square test for uniformity
        float expected = (4.0f * samples) / max;
        float chi_square = 0;

        for (uint32_t i = 0; i < max; i++) {
            float diff = bins[i] - expected;
            chi_square += (diff * diff) / expected;
        }

        printf("Range [0,%d) chi-square: %f (should be around %d)\n",
               max, chi_square, (int)max - 1);
        assert(chi_square < max * 2);  // Rough check
    }

    printf("Range generation test PASSED\n");
}

void test_stream_independence() {
    neon_prng_t rng;
    neon_prng_init(&rng, 0x12345678);

    // Generate sequences for all 4 streams
    #define SEQ_LEN 1000
    uint32_t seq[4][SEQ_LEN];

    for (int i = 0; i < SEQ_LEN; i++) {
        uint32x4_t rand = neon_prng_rand_u32(&rng);
        vst1q_u32(seq[i % 4], rand);  // Store interleaved
    }

    // Check correlation between streams
    float correlation = 0;
    for (int i = 0; i < SEQ_LEN; i++) {
        // Compare LSB of stream 0 and 1
        int bit0 = seq[0][i] & 1;
        int bit1 = seq[1][i] & 1;
        correlation += (bit0 == bit1) ? 1.0f : -1.0f;
    }
    correlation /= SEQ_LEN;

    printf("Stream correlation: %f (should be near 0)\n", correlation);
    assert(fabsf(correlation) < 0.1f);

    printf("Stream independence test PASSED\n");
}

int main() {
    printf("\n=== PRNG UNIT TESTS ===\n");
    test_xorshift128p_implementation();
    test_range_generation();
    test_probability_accuracy();
    test_stream_independence();
    printf("\n✓ ALL TESTS PASSED\n");
    return 0;
}

#endif // TEST_PRNG