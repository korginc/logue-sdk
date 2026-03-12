/**
 *  @file metal_engine.h
 *  @brief 4-operator FM metal/cymbal engine
 *
 *  Operators: All modulating each other in a cluster
 *  Fixed ratios: 1.0, 1.4, 1.7, 2.3 (inharmonic)
 *  Parameters:
 *    Param1: Inharmonicity (0-100%) - spreads ratios
 *    Param2: Brightness (0-100%) - controls high-frequency content
 */

#pragma once

#include <arm_neon.h>
#include "fm_voices.h"
#include "sine_neon.h"

// Base ratios for metal (inharmonic)
#define METAL_RATIO1 1.0f
#define METAL_RATIO2 1.4f
#define METAL_RATIO3 1.7f
#define METAL_RATIO4 2.3f

// Ratio spread range for inharmonicity parameter
#define METAL_SPREAD_MIN 1.0f
#define METAL_SPREAD_MAX 2.0f

typedef struct {
    // Four operators
    float32x4_t phase[4];
    float32x4_t base_ratio[4];  // Fixed ratios
    float32x4_t current_ratio[4]; // Modulated by inharmonicity

    // Carrier frequency (all derived from voice 0's carrier)
    float32x4_t carrier_freq_base;

    // Parameters
    float32x4_t inharmonicity;  // 0-1
    float32x4_t brightness;      // 0-1
} metal_engine_t;

/**
 * Initialize metal engine
 */
fast_inline void metal_engine_init(metal_engine_t* metal) {
    for (int i = 0; i < 4; i++) {
        metal->phase[i] = vdupq_n_f32(0.0f);
    }

    // Set base ratios
    metal->base_ratio[0] = vdupq_n_f32(METAL_RATIO1);
    metal->base_ratio[1] = vdupq_n_f32(METAL_RATIO2);
    metal->base_ratio[2] = vdupq_n_f32(METAL_RATIO3);
    metal->base_ratio[3] = vdupq_n_f32(METAL_RATIO4);

    for (int i = 0; i < 4; i++) {
        metal->current_ratio[i] = metal->base_ratio[i];
    }

    metal->carrier_freq_base = vdupq_n_f32(1000.0f);  // Mid-range default
    metal->inharmonicity = vdupq_n_f32(0.5f);
    metal->brightness = vdupq_n_f32(0.5f);
}

/**
 * Update metal engine parameters
 */
fast_inline void metal_engine_update(metal_engine_t* metal,
                                     float32x4_t param1,  // Inharmonicity
                                     float32x4_t param2) { // Brightness
    metal->inharmonicity = param1;
    metal->brightness = param2;

    // Spread ratios based on inharmonicity
    // param1=0 -> all ratios = 1.0 (harmonic)
    // param1=1 -> ratios spread to max
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t spread_range = vdupq_n_f32(METAL_SPREAD_MAX - METAL_SPREAD_MIN);

    for (int i = 0; i < 4; i++) {
        // ratio = 1.0 + (base_ratio - 1.0) * inharmonicity * spread
        float32x4_t ratio_offset = vsubq_f32(metal->base_ratio[i], one);
        float32x4_t spread_factor = vmulq_f32(metal->inharmonicity, spread_range);
        metal->current_ratio[i] = vaddq_f32(one,
                                           vmulq_f32(ratio_offset, spread_factor));
    }
}

/**
 * Set MIDI note (affects all operators proportionally)
 */
fast_inline void metal_engine_set_note(metal_engine_t* metal,
                                       uint32x4_t voice_mask,
                                       float32x4_t midi_notes) {
    // Metal typically follows pitch roughly, but inharmonic
    float32x4_t a4_freq = vdupq_n_f32(440.0f);
    float32x4_t a4_midi = vdupq_n_f32(69.0f);
    float32x4_t twelfth = vdupq_n_f32(1.0f/12.0f);

    float32x4_t exponent = vmulq_f32(vsubq_f32(midi_notes, a4_midi), twelfth);

    float32x4_t two_pow = vdupq_n_f32(1.0f);
    float32x4_t x2 = vmulq_f32(exponent, exponent);
    two_pow = vmlaq_f32(two_pow, exponent, vdupq_n_f32(0.693f));
    two_pow = vmlaq_f32(two_pow, x2, vdupq_n_f32(0.24f));

    float32x4_t base_freq = vmulq_f32(a4_freq, two_pow);

    metal->carrier_freq_base = vbslq_f32(voice_mask,
                                         base_freq,
                                         metal->carrier_freq_base);
}

/**
 * Process one sample of metal engine
 * All 4 operators modulate each other
 */
fast_inline float32x4_t metal_engine_process(metal_engine_t* metal,
                                             float32x4_t envelope,
                                             uint32x4_t active_mask) {
    float32x4_t two_pi_over_sr = vdupq_n_f32(2.0f * M_PI / 48000.0f);
    float32x4_t two_pi = vdupq_n_f32(2.0f * M_PI);

    // Calculate frequencies for all operators
    float32x4_t freq[4];
    for (int i = 0; i < 4; i++) {
        freq[i] = vmulq_f32(metal->carrier_freq_base, metal->current_ratio[i]);
    }

    // Update phases
    for (int i = 0; i < 4; i++) {
        float32x4_t inc = vmulq_f32(freq[i], two_pi_over_sr);
        metal->phase[i] = vaddq_f32(metal->phase[i], inc);

        // Wrap
        uint32x4_t wrap = vcgeq_f32(metal->phase[i], two_pi);
        metal->phase[i] = vbslq_f32(wrap,
                                    vsubq_f32(metal->phase[i], two_pi),
                                    metal->phase[i]);
    }

    // Complex modulation network:
    // op1 out -> mod op2
    // op2 out -> mod op3
    // op3 out -> mod op4
    // op4 out -> mod op1 (feedback)

    float32x4_t op_out[4];

    // First pass with initial phases
    for (int i = 0; i < 4; i++) {
        op_out[i] = neon_sin(metal->phase[i]);
    }

    // Apply modulation (simplified - just cross-modulate)
    float32x4_t mod_index = vmulq_f32(envelope, metal->brightness);

    // Op1 modulated by Op4
    float32x4_t phase1_mod = vaddq_f32(metal->phase[0],
                                       vmulq_f32(op_out[3], mod_index));
    op_out[0] = neon_sin(phase1_mod);

    // Op2 modulated by Op1
    float32x4_t phase2_mod = vaddq_f32(metal->phase[1],
                                       vmulq_f32(op_out[0], mod_index));
    op_out[1] = neon_sin(phase2_mod);

    // Op3 modulated by Op2
    float32x4_t phase3_mod = vaddq_f32(metal->phase[2],
                                       vmulq_f32(op_out[1], mod_index));
    op_out[2] = neon_sin(phase3_mod);

    // Op4 modulated by Op3
    float32x4_t phase4_mod = vaddq_f32(metal->phase[3],
                                       vmulq_f32(op_out[2], mod_index));
    op_out[3] = neon_sin(phase4_mod);

    // Mix all operators (with brightness controlling high operators)
    float32x4_t output = vmulq_f32(op_out[0], vdupq_n_f32(0.25f));
    output = vmlaq_f32(output, op_out[1], vmulq_f32(metal->brightness, vdupq_n_f32(0.25f)));
    output = vmlaq_f32(output, op_out[2], vmulq_f32(metal->brightness, vdupq_n_f32(0.3f)));
    output = vmlaq_f32(output, op_out[3], vmulq_f32(metal->brightness, vdupq_n_f32(0.4f)));

    // Apply envelope and mask
    output = vmulq_f32(output, envelope);
    output = vbslq_f32(active_mask,
                       output, vdupq_n_f32(0.0f));

    return output;
}

// ========== UNIT TEST ==========
#ifdef TEST_METAL

void test_metal_inharmonicity() {
    metal_engine_t metal;
    metal_engine_init(&metal);

    // Test with harmonic setting (inharmonicity = 0)
    float32x4_t param1 = vdupq_n_f32(0.0f);
    float32x4_t param2 = vdupq_n_f32(0.5f);
    metal_engine_update(&metal, param1, param2);

    // All ratios should be 1.0
    for (int i = 0; i < 4; i++) {
        float ratio = vgetq_lane_f32(metal.current_ratio[i], 0);
        assert(fabsf(ratio - 1.0f) < 0.001f);
    }

    // Test with full inharmonicity
    param1 = vdupq_n_f32(1.0f);
    metal_engine_update(&metal, param1, param2);

    // Ratios should spread
    float r1 = vgetq_lane_f32(metal.current_ratio[0], 0);
    float r2 = vgetq_lane_f32(metal.current_ratio[3], 0);
    assert(r2 > r1);

    printf("Metal engine inharmonicity test PASSED\n");
}

#endif