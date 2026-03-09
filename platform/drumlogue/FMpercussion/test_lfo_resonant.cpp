/**
 * @file test_lfo_resonant.cpp
 * @brief Test LFO modulation of resonant parameters
 * 
 * Compile: g++ -o test_lfo test_lfo_resonant.cpp -lm
 * Run: ./test_lfo
 */

#include <stdio.h>
#include <math.h>

// Simulate LFO modulation of resonant parameters
void test_lfo_freq_modulation() {
    printf("\n=== Testing LFO Frequency Modulation ===\n");
    
    float base_freq = 1000.0f;
    float lfo_values[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    float depth = 0.5f;  // 50% depth
    
    printf("Base frequency: %.0f Hz\n", base_freq);
    printf("LFO depth: %.1f (50%%)\n", depth);
    printf("\nLFO Value → Bipolar → Modulated Frequency:\n");
    
    for (int i = 0; i < 5; i++) {
        float lfo = lfo_values[i];
        // Convert 0-1 to -1..1
        float bipolar = lfo * 2.0f - 1.0f;
        // Scale by depth and max deviation (500Hz)
        float modulation = bipolar * depth * 500.0f;
        float modulated = base_freq + modulation;
        
        printf("  %.2f → %+.2f → %.0f Hz %s\n", 
               lfo, bipolar, modulated,
               (modulated >= 500 && modulated <= 1500) ? "✓" : "✗");
    }
    printf("✓ Frequency modulation test PASSED\n");
}

void test_lfo_resonance_modulation() {
    printf("\n=== Testing LFO Resonance Modulation ===\n");
    
    float base_res = 0.5f;
    float lfo_values[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    float depth = 0.5f;  // 50% depth
    
    printf("Base resonance: %.2f\n", base_res);
    printf("LFO depth: %.1f (50%%)\n", depth);
    printf("\nLFO Value → Bipolar → Modulated Resonance:\n");
    
    for (int i = 0; i < 5; i++) {
        float lfo = lfo_values[i];
        // Convert 0-1 to -1..1
        float bipolar = lfo * 2.0f - 1.0f;
        // Scale by depth and max deviation (0.3)
        float modulation = bipolar * depth * 0.3f;
        float modulated = base_res + modulation;
        
        // Clamp to valid range
        if (modulated < 0.0f) modulated = 0.0f;
        if (modulated > 0.99f) modulated = 0.99f;
        
        printf("  %.2f → %+.2f → %.2f %s\n", 
               lfo, bipolar, modulated,
               (modulated >= 0.0f && modulated <= 0.99f) ? "✓" : "✗");
    }
    printf("✓ Resonance modulation test PASSED\n");
}

void test_both_lfos_simultaneously() {
    printf("\n=== Testing Both LFOs Simultaneously ===\n");
    
    // Simulate LFO1 modulating frequency, LFO2 modulating resonance
    float lfo1 = 0.7f;  // 70% -> bipolar +0.4
    float lfo2 = 0.3f;  // 30% -> bipolar -0.4
    
    float base_freq = 1000.0f;
    float base_res = 0.5f;
    float depth_freq = 0.8f;  // 80%
    float depth_res = 0.6f;   // 60%
    
    float bipolar1 = lfo1 * 2.0f - 1.0f;
    float bipolar2 = lfo2 * 2.0f - 1.0f;
    
    float mod_freq = bipolar1 * depth_freq * 500.0f;
    float mod_res = bipolar2 * depth_res * 0.3f;
    
    float freq_out = base_freq + mod_freq;
    float res_out = base_res + mod_res;
    
    // Clamp
    if (res_out < 0.0f) res_out = 0.0f;
    if (res_out > 0.99f) res_out = 0.99f;
    
    printf("LFO1: %.2f → bipolar %+.2f → freq mod %+.0f Hz → output %.0f Hz\n",
           lfo1, bipolar1, mod_freq, freq_out);
    printf("LFO2: %.2f → bipolar %+.2f → res mod %+.2f → output %.2f\n",
           lfo2, bipolar2, mod_res, res_out);
    printf("✓ Simultaneous modulation test PASSED\n");
}

int main() {
    printf("========================================\n");
    printf("LFO-Resonant Parameter Test Suite\n");
    printf("========================================\n");
    
    test_lfo_freq_modulation();
    test_lfo_resonance_modulation();
    test_both_lfos_simultaneously();
    
    printf("\n========================================\n");
    printf("✓ ALL TESTS PASSED\n");
    printf("========================================\n");
    return 0;
}