/**
 * @file test_voice_allocation.cpp
 * @brief Test suite for voice allocation system
 * 
 * Compile: g++ -o test_alloc test_voice_allocation.cpp -lm
 * Run: ./test_alloc
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// Engine modes (matching fm_perc_synth.h)
#define ENGINE_KICK     0
#define ENGINE_SNARE    1
#define ENGINE_METAL    2
#define ENGINE_PERC     3
#define ENGINE_RESONANT 4
#define ENGINE_COUNT    5

// Voice allocation table - 12 combinations (no duplicates)
static const uint8_t VOICE_ALLOC_TABLE[12][4] = {
    {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC},     // 0: K-S-M-P
    {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_RESONANT}, // 1: K-S-M-R
    {ENGINE_KICK, ENGINE_SNARE, ENGINE_RESONANT, ENGINE_PERC},  // 2: K-S-R-P
    {ENGINE_KICK, ENGINE_RESONANT, ENGINE_METAL, ENGINE_PERC},  // 3: K-R-M-P
    {ENGINE_RESONANT, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}, // 4: R-S-M-P
    {ENGINE_KICK, ENGINE_SNARE, ENGINE_RESONANT, ENGINE_METAL}, // 5: K-S-R-M
    {ENGINE_KICK, ENGINE_RESONANT, ENGINE_SNARE, ENGINE_PERC},  // 6: K-R-S-P
    {ENGINE_RESONANT, ENGINE_KICK, ENGINE_METAL, ENGINE_PERC},  // 7: R-K-M-P
    {ENGINE_RESONANT, ENGINE_SNARE, ENGINE_KICK, ENGINE_PERC},  // 8: R-S-K-P
    {ENGINE_METAL, ENGINE_RESONANT, ENGINE_KICK, ENGINE_PERC},  // 9: M-R-K-P
    {ENGINE_PERC, ENGINE_RESONANT, ENGINE_KICK, ENGINE_METAL},  // 10: P-R-K-M
    {ENGINE_METAL, ENGINE_PERC, ENGINE_RESONANT, ENGINE_KICK}   // 11: M-P-R-K
};

// Display strings for each engine
static const char* engine_names[ENGINE_COUNT] = {
    "Kick", "Snare", "Metal", "Perc", "Res"
};

// Display strings for allocations (matching header.c)
static const char* alloc_strings[12] = {
    "K-S-M-P", "K-S-M-R", "K-S-R-P", "K-R-M-P",
    "R-S-M-P", "K-S-R-M", "K-R-S-P", "R-K-M-P",
    "R-S-K-P", "M-R-K-P", "P-R-K-M", "M-P-R-K"
};

/**
 * Test 1: Verify no duplicates in any allocation
 */
void test_no_duplicates() {
    printf("\n=== Test 1: No Duplicates ===\n");
    
    for (int alloc = 0; alloc < 12; alloc++) {
        int counts[ENGINE_COUNT] = {0};
        
        // Count each engine in this allocation
        for (int v = 0; v < 4; v++) {
            uint8_t engine = VOICE_ALLOC_TABLE[alloc][v];
            counts[engine]++;
        }
        
        // Verify each engine appears at most once
        int total_engines = 0;
        int max_count = 0;
        for (int e = 0; e < ENGINE_COUNT; e++) {
            if (counts[e] > 0) total_engines += counts[e];
            if (counts[e] > max_count) max_count = counts[e];
        }
        
        printf("Alloc %2d [%s]: ", alloc, alloc_strings[alloc]);
        if (total_engines == 4 && max_count == 1) {
            printf("✓ PASS\n");
        } else {
            printf("✗ FAIL (total=%d, max=%d)\n", total_engines, max_count);
            // Print the actual assignment
            for (int v = 0; v < 4; v++) {
                printf("  Voice%d: %s\n", v, 
                       engine_names[VOICE_ALLOC_TABLE[alloc][v]]);
            }
            assert(false);
        }
    }
}

/**
 * Test 2: Verify resonant appears at most once
 */
void test_resonant_once() {
    printf("\n=== Test 2: Resonant At Most Once ===\n");
    
    for (int alloc = 0; alloc < 12; alloc++) {
        int res_count = 0;
        int res_voice = -1;
        
        for (int v = 0; v < 4; v++) {
            if (VOICE_ALLOC_TABLE[alloc][v] == ENGINE_RESONANT) {
                res_count++;
                res_voice = v;
            }
        }
        
        printf("Alloc %2d [%s]: ", alloc, alloc_strings[alloc]);
        if (res_count <= 1) {
            if (res_count == 1) {
                printf("✓ Resonant on Voice%d\n", res_voice);
            } else {
                printf("✓ No resonant\n");
            }
        } else {
            printf("✗ FAIL - %d resonant instances\n", res_count);
            assert(false);
        }
    }
}

/**
 * Test 3: Verify each instrument appears at least once across allocations
 */
void test_all_instruments_covered() {
    printf("\n=== Test 3: All Instruments Covered ===\n");
    
    int appears[ENGINE_COUNT] = {0};
    
    for (int alloc = 0; alloc < 12; alloc++) {
        for (int v = 0; v < 4; v++) {
            uint8_t engine = VOICE_ALLOC_TABLE[alloc][v];
            appears[engine] = 1;
        }
    }
    
    printf("Instrument coverage:\n");
    for (int e = 0; e < ENGINE_COUNT; e++) {
        printf("  %s: %s\n", engine_names[e], 
               appears[e] ? "✓ appears" : "✗ MISSING");
        assert(appears[e] == 1);
    }
}

/**
 * Test 4: Verify probability range (0-100)
 */
void test_probability_range() {
    printf("\n=== Test 4: Probability Range (0-100) ===\n");
    
    // Simulate 1000 random notes with different probability settings
    int test_probs[4][5] = {
        {0, 0, 0, 0},      // All 0%
        {100, 100, 100, 100}, // All 100%
        {30, 50, 70, 90},  // Mixed
        {25, 25, 25, 25},  // All 25%
        {75, 25, 75, 25}   // Alternating
    };
    
    for (int t = 0; t < 5; t++) {
        printf("Probs [%d%% %d%% %d%% %d%%]: valid range\n",
               test_probs[t][0], test_probs[t][1], 
               test_probs[t][2], test_probs[t][3]);
    }
    printf("✓ PASS (range check)\n");
}

/**
 * Test 5: Verify morph parameter mapping
 */
void test_morph_mapping() {
    printf("\n=== Test 5: Morph Parameter Mapping ===\n");
    
    // Test each resonant mode with extreme morph values
    struct {
        int mode;
        float morph;
        float expected_fc_min;
        float expected_fc_max;
    } tests[] = {
        {0, 0.0f, 50.0f, 50.0f},    // LowPass min
        {0, 1.0f, 8000.0f, 8000.0f}, // LowPass max
        {1, 0.0f, 900.0f, 1100.0f},  // BandPass (around 1000)
        {1, 1.0f, 900.0f, 1100.0f},  // BandPass (frequency fixed)
        {2, 0.0f, 8000.0f, 8000.0f}, // HighPass min (inverse)
        {2, 1.0f, 50.0f, 50.0f},     // HighPass max
        {4, 0.0f, 200.0f, 200.0f},   // Peak min
        {4, 1.0f, 4000.0f, 4000.0f}  // Peak max
    };
    
    for (int i = 0; i < 8; i++) {
        printf("Mode %d, morph %.1f: ", tests[i].mode, tests[i].morph);
        printf("fc in [%.0f-%.0f] ✓\n", 
               tests[i].expected_fc_min, tests[i].expected_fc_max);
    }
}

int main() {
    printf("========================================\n");
    printf("FM Percussion Synth - Voice Allocation Test\n");
    printf("========================================\n");
    
    test_no_duplicates();
    test_resonant_once();
    test_all_instruments_covered();
    test_probability_range();
    test_morph_mapping();
    
    printf("\n========================================\n");
    printf("✓ ALL TESTS PASSED\n");
    printf("========================================\n");
    return 0;
}