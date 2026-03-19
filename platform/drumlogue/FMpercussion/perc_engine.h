/**
 *  @file perc_engine.h
 *  @brief 3-operator FM percussion/tom engine
 *
 *  Operators:
 *    Op1: Carrier
 *    Op2: Modulator 1 (ratio center)
 *    Op3: Modulator 2 (variation)
 *  Parameters:
 *    Param1: Ratio center (0-100%) - maps to 1.0-3.0
 *    Param2: Variation (0-100%) - secondary modulation amount
 */

#pragma once

#include <arm_neon.h>
#include "sine_neon.h"
#include "fm_voices.h"

#define PERC_RATIO_MIN 1.0f
#define PERC_RATIO_MAX 3.0f
#define PERC_VARIATION_MAX 1.5f

typedef struct {
    // Three operators
    float32x4_t phase[3];
    float32x4_t freq_base;     // Carrier base frequency
    float32x4_t ratio_center;   // Main modulator ratio
    float32x4_t variation;       // Secondary modulation amount

    // Parameters
    float32x4_t ratio_param;    // 0-1 mapped to ratio range
    float32x4_t var_param;      // 0-1 mapped to variation range
} perc_engine_t;

/**
 * Initialize perc engine
 */
fast_inline void perc_engine_init(perc_engine_t* perc) {
    for (int i = 0; i < 3; i++) {
        perc->phase[i] = vdupq_n_f32(0.0f);
    }

    perc->freq_base = vdupq_n_f32(200.0f);  // Mid tom default
    perc->ratio_center = vdupq_n_f32(2.0f);
    perc->variation = vdupq_n_f32(0.5f);

    perc->ratio_param = vdupq_n_f32(0.5f);
    perc->var_param = vdupq_n_f32(0.5f);
}

/**
 * Update perc engine parameters
 */
fast_inline void perc_engine_update(perc_engine_t* perc,
                                    float32x4_t param1,  // Ratio center
                                    float32x4_t param2) { // Variation
    perc->ratio_param = param1;
    perc->var_param = param2;

    // Map param1 (0-1) to ratio range (1.0-3.0)
    float32x4_t ratio_range = vdupq_n_f32(PERC_RATIO_MAX - PERC_RATIO_MIN);
    perc->ratio_center = vaddq_f32(vdupq_n_f32(PERC_RATIO_MIN),
                                   vmulq_f32(param1, ratio_range));

    // Map param2 to variation amount
    perc->variation = vmulq_f32(param2, vdupq_n_f32(PERC_VARIATION_MAX));
}

/**
 * Set MIDI note (tunable percussion)
 */
fast_inline void perc_engine_set_note(perc_engine_t* perc,
                                      uint32x4_t voice_mask,
                                      float32x4_t midi_notes) {
    float32x4_t a4_freq = vdupq_n_f32(440.0f);
    float32x4_t a4_midi = vdupq_n_f32(69.0f);
    float32x4_t twelfth = vdupq_n_f32(1.0f/12.0f);

    float32x4_t exponent = vmulq_f32(vsubq_f32(midi_notes, a4_midi), twelfth);

    float32x4_t two_pow = vdupq_n_f32(1.0f);
    float32x4_t x2 = vmulq_f32(exponent, exponent);
    two_pow = vmlaq_f32(two_pow, exponent, vdupq_n_f32(0.693f));
    two_pow = vmlaq_f32(two_pow, x2, vdupq_n_f32(0.24f));

    float32x4_t base_freq = vmulq_f32(a4_freq, two_pow);

    perc->freq_base = vbslq_f32(voice_mask,
                                base_freq,
                                perc->freq_base);
}

/**
 * Process one sample of perc engine
 */
fast_inline float32x4_t perc_engine_process(perc_engine_t* perc,
                                            float32x4_t envelope,
                                            uint32x4_t active_mask) {
    float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI / 48000.0f);
    float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);

    // Calculate frequencies
    float32x4_t carrier_freq = perc->freq_base;
    float32x4_t mod1_freq = vmulq_f32(carrier_freq, perc->ratio_center);

    // Modulator 2 frequency varies with envelope for dynamic timbre
    float32x4_t mod2_ratio = vaddq_f32(perc->ratio_center,
                                       vmulq_f32(perc->variation, envelope));
    float32x4_t mod2_freq = vmulq_f32(carrier_freq, mod2_ratio);

    // Phase increments
    float32x4_t inc[3] = {
        vmulq_f32(carrier_freq, two_pi_over_sr),
        vmulq_f32(mod1_freq, two_pi_over_sr),
        vmulq_f32(mod2_freq, two_pi_over_sr)
    };

    // Update phases
    for (int i = 0; i < 3; i++) {
        perc->phase[i] = vaddq_f32(perc->phase[i], inc[i]);

        // Wrap
        uint32x4_t wrap = vcgeq_f32(perc->phase[i], two_pi);
        perc->phase[i] = vbslq_f32(wrap,
                                   vsubq_f32(perc->phase[i], two_pi),
                                   perc->phase[i]);
    }

    // Generate modulator signals
    float32x4_t mod1 = neon_sin(perc->phase[1]);
    float32x4_t mod2 = neon_sin(perc->phase[2]);

    // Complex modulation: carrier modulated by both modulators
    float32x4_t modulation = vaddq_f32(vmulq_f32(mod1, envelope),
                                       vmulq_f32(mod2, vmulq_f32(envelope, envelope)));

    float32x4_t modulated_phase = vaddq_f32(perc->phase[0],
                                           vmulq_f32(modulation, envelope));

    float32x4_t output = neon_sin(modulated_phase);

    // Apply envelope and mask
    output = vmulq_f32(output, envelope);
    output = vbslq_f32(active_mask,
                       output, vdupq_n_f32(0.0f));

    return output;
}

// ========== UNIT TEST ==========
#ifdef TEST_PERC

void test_perc_ratio_range() {
    perc_engine_t perc;
    perc_engine_init(&perc);

    // Test ratio center mapping
    float32x4_t param1 = vdupq_n_f32(0.0f);  // Min ratio
    float32x4_t param2 = vdupq_n_f32(0.5f);
    perc_engine_update(&perc, param1, param2);

    float ratio = vgetq_lane_f32(perc.ratio_center, 0);
    assert(fabsf(ratio - PERC_RATIO_MIN) < 0.001f);

    param1 = vdupq_n_f32(1.0f);  // Max ratio
    perc_engine_update(&perc, param1, param2);

    ratio = vgetq_lane_f32(perc.ratio_center, 0);
    assert(fabsf(ratio - PERC_RATIO_MAX) < 0.001f);

    printf("Perc engine ratio range test PASSED\n");
}

#endif