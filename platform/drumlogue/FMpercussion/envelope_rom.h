/**
 *  @file envelope_rom.h
 *  @brief 128 predefined ADR envelopes for percussion
 *
 *  Single parameter selects from 128 carefully tuned curves
 *  Attack: 0-50ms, Decay: 20-500ms, Release: 10-500ms
 */

#pragma once

#include <arm_neon.h>
#include <stdint.h>

typedef struct {
    uint8_t attack_ms;    // 0-50ms
    uint8_t decay_ms;     // 20-500ms
    uint8_t release_ms;   // 10-500ms
    uint8_t curve_type;   // 0=linear, 1=exponential
} env_curve_t;

// 128 predefined envelope curves
static const env_curve_t ENV_ROM[128] = {
    // Range 0-15: Tight & Dry (fast attack, short decay)
    {0, 20, 10, 1},   // 0: Fastest click
    {0, 30, 15, 1},   // 1
    {0, 40, 20, 1},   // 2
    {0, 50, 25, 1},   // 3
    {1, 60, 30, 1},   // 4
    {1, 70, 35, 1},   // 5
    {1, 80, 40, 1},   // 6
    {1, 90, 45, 1},   // 7
    {2, 100, 50, 1},  // 8
    {2, 110, 55, 1},  // 9
    {2, 120, 60, 1},  // 10
    {2, 130, 65, 1},  // 11
    {3, 140, 70, 1},  // 12
    {3, 150, 75, 1},  // 13
    {3, 160, 80, 1},  // 14
    {3, 170, 85, 1},  // 15

    // Range 16-31: Punchy (standard drums)
    {4, 180, 90, 1},   // 16
    {4, 190, 95, 1},   // 17
    {5, 200, 100, 1},  // 18
    {5, 210, 105, 1},  // 19
    {6, 220, 110, 1},  // 20
    {6, 230, 115, 1},  // 21
    {7, 240, 120, 1},  // 22
    {7, 250, 125, 1},  // 23
    {8, 260, 130, 1},  // 24
    {8, 270, 135, 1},  // 25
    {9, 280, 140, 1},  // 26
    {9, 290, 145, 1},  // 27
    {10, 300, 150, 1}, // 28
    {10, 310, 155, 1}, // 29
    {11, 320, 160, 1}, // 30
    {11, 330, 165, 1}, // 31

    // Range 32-47: Fat (bigger sounds)
    {12, 340, 170, 1}, // 32
    {12, 350, 175, 1}, // 33
    {13, 360, 180, 1}, // 34
    {13, 370, 185, 1}, // 35
    {14, 380, 190, 1}, // 36
    {14, 390, 195, 1}, // 37
    {15, 400, 200, 1}, // 38
    {15, 410, 205, 1}, // 39
    {16, 420, 210, 1}, // 40
    {16, 430, 215, 1}, // 41
    {17, 440, 220, 1}, // 42
    {17, 450, 225, 1}, // 43
    {18, 460, 230, 1}, // 44
    {18, 470, 235, 1}, // 45
    {19, 480, 240, 1}, // 46
    {19, 490, 245, 1}, // 47

    // Range 48-63: Boomy (resonant)
    {20, 500, 250, 1}, // 48
    {20, 500, 260, 1}, // 49
    {21, 500, 270, 1}, // 50
    {21, 500, 280, 1}, // 51
    {22, 500, 290, 1}, // 52
    {22, 500, 300, 1}, // 53
    {23, 500, 310, 1}, // 54
    {23, 500, 320, 1}, // 55
    {24, 500, 330, 1}, // 56
    {24, 500, 340, 1}, // 57
    {25, 500, 350, 1}, // 58
    {25, 500, 360, 1}, // 59
    {26, 500, 370, 1}, // 60
    {26, 500, 380, 1}, // 61
    {27, 500, 390, 1}, // 62
    {27, 500, 400, 1}, // 63

    // Range 64-79: Gated (quick decay, long release)
    {0, 50, 200, 1},   // 64
    {0, 60, 220, 1},   // 65
    {1, 70, 240, 1},   // 66
    {1, 80, 260, 1},   // 67
    {2, 90, 280, 1},   // 68
    {2, 100, 300, 1},  // 69
    {3, 110, 320, 1},  // 70
    {3, 120, 340, 1},  // 71
    {4, 130, 360, 1},  // 72
    {4, 140, 380, 1},  // 73
    {5, 150, 400, 1},  // 74
    {5, 160, 420, 1},  // 75
    {6, 170, 440, 1},  // 76
    {6, 180, 460, 1},  // 77
    {7, 190, 480, 1},  // 78
    {7, 200, 500, 1},  // 79

    // Range 80-95: Ambient (washy)
    {10, 250, 400, 0}, // 80
    {11, 260, 410, 0}, // 81
    {12, 270, 420, 0}, // 82
    {13, 280, 430, 0}, // 83
    {14, 290, 440, 0}, // 84
    {15, 300, 450, 0}, // 85
    {16, 310, 460, 0}, // 86
    {17, 320, 470, 0}, // 87
    {18, 330, 480, 0}, // 88
    {19, 340, 490, 0}, // 89
    {20, 350, 500, 0}, // 90
    {21, 360, 500, 0}, // 91
    {22, 370, 500, 0}, // 92
    {23, 380, 500, 0}, // 93
    {24, 390, 500, 0}, // 94
    {25, 400, 500, 0}, // 95

    // Range 96-111: Reverse-like (slow attack)
    {30, 200, 100, 0}, // 96
    {32, 210, 110, 0}, // 97
    {34, 220, 120, 0}, // 98
    {36, 230, 130, 0}, // 99
    {38, 240, 140, 0}, // 100
    {40, 250, 150, 0}, // 101
    {42, 260, 160, 0}, // 102
    {44, 270, 170, 0}, // 103
    {46, 280, 180, 0}, // 104
    {48, 290, 190, 0}, // 105
    {50, 300, 200, 0}, // 106
    {50, 310, 210, 0}, // 107
    {50, 320, 220, 0}, // 108
    {50, 330, 230, 0}, // 109
    {50, 340, 240, 0}, // 110
    {50, 350, 250, 0}, // 111

    // Range 112-127: Pad-like (sustained)
    {20, 400, 400, 0}, // 112
    {22, 410, 410, 0}, // 113
    {24, 420, 420, 0}, // 114
    {26, 430, 430, 0}, // 115
    {28, 440, 440, 0}, // 116
    {30, 450, 450, 0}, // 117
    {32, 460, 460, 0}, // 118
    {34, 470, 470, 0}, // 119
    {36, 480, 480, 0}, // 120
    {38, 490, 490, 0}, // 121
    {40, 500, 500, 0}, // 122
    {42, 500, 500, 0}, // 123
    {44, 500, 500, 0}, // 124
    {46, 500, 500, 0}, // 125
    {48, 500, 500, 0}, // 126
    {50, 500, 500, 0}  // 127
};

/**
 * Get envelope parameters for a given shape index
 */
fast_inline void get_envelope(uint8_t shape_idx, 
                              uint8_t* attack_ms,
                              uint8_t* decay_ms,
                              uint8_t* release_ms,
                              uint8_t* curve_type) {
    const env_curve_t* env = &ENV_ROM[shape_idx];
    *attack_ms = env->attack_ms;
    *decay_ms = env->decay_ms;
    *release_ms = env->release_ms;
    *curve_type = env->curve_type;
}

/**
 * NEON-optimized envelope generator for 4 voices
 */
typedef struct {
    float32x4_t level;        // Current envelope level (0-1)
    uint32x4_t stage;         // 0=attack, 1=decay, 2=release, 3=off
    uint32x4_t samples_left;  // Samples remaining in current stage
    float32x4_t increment;    // Per-sample increment for linear stages
} neon_envelope_t;

/**
 * Initialize envelope generator
 */
fast_inline void neon_envelope_init(neon_envelope_t* env) {
    env->level = vdupq_n_f32(0.0f);
    env->stage = vdupq_n_u32(3);  // Off
    env->samples_left = vdupq_n_u32(0);
    env->increment = vdupq_n_f32(0.0f);
}

/**
 * Trigger envelope for selected voices
 */
fast_inline void neon_envelope_trigger(neon_envelope_t* env,
                                       uint32x4_t voice_mask,
                                       uint8_t shape_idx) {
    uint8_t a, d, r, curve;
    get_envelope(shape_idx, &a, &d, &r, &curve);
    
    // Convert ms to samples at 48kHz
    uint32_t attack_samples = a * 48;
    uint32_t decay_samples = d * 48;
    uint32_t release_samples = r * 48;
    
    // Set up attack stage for triggered voices
    uint32x4_t attack_stage = vdupq_n_u32(0);
    uint32x4_t attack_samples_vec = vdupq_n_u32(attack_samples);
    
    // Calculate attack increment (linear: 1.0 / samples)
    float attack_inc = 1.0f / (attack_samples ? attack_samples : 1);
    float32x4_t attack_inc_vec = vdupq_n_f32(attack_inc);
    
    // Apply to masked voices only
    env->stage = vbslq_u32(voice_mask, attack_stage, env->stage);
    env->samples_left = vbslq_u32(voice_mask, attack_samples_vec, env->samples_left);
    env->increment = vbslq_f32(vreinterpretq_f32_u32(voice_mask), 
                                attack_inc_vec, env->increment);
    env->level = vbslq_f32(vreinterpretq_f32_u32(voice_mask),
                           vdupq_n_f32(0.0f), env->level);
}

/**
 * Process one sample of envelope for all 4 voices
 */
fast_inline void neon_envelope_process(neon_envelope_t* env) {
    // Update level based on current stage
    env->level = vaddq_f32(env->level, env->increment);
    
    // Decrement sample counters
    env->samples_left = vsubq_u32(env->samples_left, vdupq_n_u32(1));
    
    // Check for stage completion
    uint32x4_t stage_done = vceqq_u32(env->samples_left, vdupq_n_u32(0));
    
    // Handle stage transitions
    uint32x4_t attack_stage = vceqq_u32(env->stage, vdupq_n_u32(0));
    uint32x4_t decay_stage = vceqq_u32(env->stage, vdupq_n_u32(1));
    uint32x4_t release_stage = vceqq_u32(env->stage, vdupq_n_u32(2));
    
    // Attack -> Decay transition
    uint32x4_t attack_done = vandq_u32(stage_done, attack_stage);
    if (vget_lane_u32(vreinterpret_u32_u8(vmovn_u32(attack_done)), 0)) {
        // Move to decay stage
        env->stage = vbslq_u32(attack_done, vdupq_n_u32(1), env->stage);
        
        // Set up decay (exponential)
        // For now, linear decay - will add exponential later
        float decay_samples = 100 * 48;  // Placeholder
        env->samples_left = vbslq_u32(attack_done,
                                      vdupq_n_u32((uint32_t)decay_samples),
                                      env->samples_left);
        env->increment = vbslq_f32(vreinterpretq_f32_u32(attack_done),
                                   vdupq_n_f32(-1.0f / decay_samples),
                                   env->increment);
    }
    
    // Clamp level to [0,1]
    env->level = vmaxq_f32(vdupq_n_f32(0.0f), 
                           vminq_f32(vdupq_n_f32(1.0f), env->level));
}

// ========== UNIT TEST ==========
#ifdef TEST_ENVELOPE

void test_envelope_rom() {
    // Test a few envelope shapes
    uint8_t a, d, r, c;
    
    get_envelope(0, &a, &d, &r, &c);
    assert(a == 0 && d == 20 && r == 10);
    
    get_envelope(48, &a, &d, &r, &c);
    assert(a == 20 && d == 500 && r == 250);
    
    get_envelope(127, &a, &d, &r, &c);
    assert(a == 50 && d == 500 && r == 500);
    
    printf("Envelope ROM test PASSED\n");
}

#endif