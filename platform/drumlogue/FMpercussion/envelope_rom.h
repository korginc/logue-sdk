#pragma once

/**
 * @file envelope_rom.h
 * @brief 128 predefined ADR envelopes for percussion
 *
 * Fixed version with proper parallel processing for all 4 voices
 * Attack → Decay → Release (ADR) - no sustain
 */

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"

// Envelope stages
#define ENV_STAGE_ATTACK  0
#define ENV_STAGE_DECAY   1
#define ENV_STAGE_RELEASE 2
#define ENV_STATE_OFF     3

// Envelope curve types
#define ENV_CURVE_LINEAR    0
#define ENV_CURVE_EXPONENTIAL 1

typedef struct {
    uint8_t attack_ms;    // 0-50ms
    uint8_t decay_ms;     // 20-500ms
    uint8_t release_ms;   // 10-500ms
    uint8_t curve_type;   // 0=linear, 1=exponential
} env_curve_t;

// 128 predefined envelope curves (as previously defined)
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
 * NEON-optimized envelope generator for 4 parallel voices
 */
typedef struct {
    float32x4_t level;           // Current envelope level (0-1) for 4 voices
    uint32x4_t stage;            // 0=attack, 1=decay, 2=release, 3=off
    uint32x4_t samples_left;     // Samples remaining in current stage
    float32x4_t increment;       // Per-sample increment for current stage
    float32x4_t attack_samples;  // Attack length in samples (pre-calculated)
    float32x4_t decay_samples;   // Decay length in samples
    float32x4_t release_samples; // Release length in samples
    uint32x4_t curve_type;       // 0=linear, 1=exponential
} neon_envelope_t;

/**
 * Initialize envelope generator
 */
fast_inline void neon_envelope_init(neon_envelope_t* env) {
    env->level = vdupq_n_f32(0.0f);
    env->stage = vdupq_n_u32(ENV_STATE_OFF);
    env->samples_left = vdupq_n_u32(0);
    env->increment = vdupq_n_f32(0.0f);
    env->attack_samples = vdupq_n_f32(0.0f);
    env->decay_samples = vdupq_n_f32(0.0f);
    env->release_samples = vdupq_n_f32(0.0f);
    env->curve_type = vdupq_n_u32(ENV_CURVE_LINEAR);
}

/**
 * Get envelope parameters from ROM
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
 * Trigger envelope for selected voices
 * @param env Envelope state
 * @param voice_mask Which voices to trigger (1 bit per voice)
 * @param shape_idx Envelope shape index (0-127)
 */
fast_inline void neon_envelope_trigger(neon_envelope_t* env,
                                       uint32x4_t voice_mask,
                                       uint8_t shape_idx) {
    uint8_t a_ms, d_ms, r_ms, curve;
    get_envelope(shape_idx, &a_ms, &d_ms, &r_ms, &curve);

    // Convert ms to samples at 48kHz
    float attack_samps = a_ms * 48.0f;
    float decay_samps = d_ms * 48.0f;
    float release_samps = r_ms * 48.0f;

    // Ensure non-zero to avoid division by zero
    if (attack_samps < 1.0f) attack_samps = 1.0f;
    if (decay_samps < 1.0f) decay_samps = 1.0f;
    if (release_samps < 1.0f) release_samps = 1.0f;

    // Store pre-calculated stage lengths
    env->attack_samples = vbslq_f32(voice_mask,
                                    vdupq_n_f32(attack_samps),
                                    env->attack_samples);
    env->decay_samples = vbslq_f32(voice_mask,
                                   vdupq_n_f32(decay_samps),
                                   env->decay_samples);
    env->release_samples = vbslq_f32(voice_mask,
                                     vdupq_n_f32(release_samps),
                                     env->release_samples);

    // Set curve type
    env->curve_type = vbslq_u32(voice_mask,
                                vdupq_n_u32(curve),
                                env->curve_type);

    // Set stage to attack
    env->stage = vbslq_u32(voice_mask,
                           vdupq_n_u32(ENV_STAGE_ATTACK),
                           env->stage);

    // Set samples left for attack stage
    env->samples_left = vbslq_u32(voice_mask,
                                  vcvtq_u32_f32(vdupq_n_f32(attack_samps)),
                                  env->samples_left);

    // Set increment for attack (linear: positive slope to reach 1.0)
    // For exponential, we'll handle differently in process function
    env->increment = vbslq_f32(voice_mask,
                               vdupq_n_f32(1.0f / attack_samps),
                               env->increment);

    // Reset level to 0 for triggered voices
    env->level = vbslq_f32(voice_mask,
                           vdupq_n_f32(0.0f),
                           env->level);
}

/**
 * Process one sample for all 4 voices in parallel
 * Must be called exactly once per sample
 */
fast_inline void neon_envelope_process(neon_envelope_t* env) {
    // -----------------------------------------------------------------
    // 1. Update level based on current stage and curve type
    // -----------------------------------------------------------------
    // For linear stages: level += increment
    // For exponential stages: we need different handling

    // First, handle linear stages (most common)
    env->level = vaddq_f32(env->level, env->increment);

    // For exponential attack, we'd need a different approach
    // But for simplicity, we'll use linear for now and add exp option later

    // -----------------------------------------------------------------
    // 2. Decrement sample counters for all voices
    // -----------------------------------------------------------------
    env->samples_left = vsubq_u32(env->samples_left, vdupq_n_u32(1));

    // -----------------------------------------------------------------
    // 3. Check which voices have completed their current stage
    // -----------------------------------------------------------------
    uint32x4_t stage_done = vceqq_u32(env->samples_left, vdupq_n_u32(0));

    // -----------------------------------------------------------------
    // 4. Handle stage transitions for ALL voices that are done
    // -----------------------------------------------------------------
    // Instead of checking only first voice with vget_lane, we process all lanes
    // using vector operations

    // Get current stage masks
    uint32x4_t attack_stage = vceqq_u32(env->stage, vdupq_n_u32(ENV_STAGE_ATTACK));
    uint32x4_t decay_stage = vceqq_u32(env->stage, vdupq_n_u32(ENV_STAGE_DECAY));
    uint32x4_t release_stage = vceqq_u32(env->stage, vdupq_n_u32(ENV_STAGE_RELEASE));

    // -----------------------------------------------------------------
    // 4a. Attack -> Decay transition (for voices that finished attack)
    // -----------------------------------------------------------------
    uint32x4_t attack_done = vandq_u32(stage_done, attack_stage);

    // Update stage to decay for voices that finished attack
    env->stage = vbslq_u32(attack_done,
                           vdupq_n_u32(ENV_STAGE_DECAY),
                           env->stage);

    // Set samples left for decay stage
    env->samples_left = vbslq_u32(attack_done,
                                  vcvtq_u32_f32(env->decay_samples),
                                  env->samples_left);

    // Set increment for decay (negative slope to reach 0)
    // For linear decay: increment = -1.0 / decay_samples
    env->increment = vbslq_f32(vreinterpretq_f32_u32(attack_done),
                               vnegq_f32(vrecpeq_f32(env->decay_samples)),
                               env->increment);

    // -----------------------------------------------------------------
    // 4b. Decay -> Release transition (for voices that finished decay)
    // -----------------------------------------------------------------
    uint32x4_t decay_done = vandq_u32(stage_done, decay_stage);

    // Update stage to release for voices that finished decay
    env->stage = vbslq_u32(decay_done,
                           vdupq_n_u32(ENV_STAGE_RELEASE),
                           env->stage);

    // Set samples left for release stage
    env->samples_left = vbslq_u32(decay_done,
                                  vcvtq_u32_f32(env->release_samples),
                                  env->samples_left);

    // Set increment for release (continues negative slope to reach 0)
    env->increment = vbslq_f32(vreinterpretq_f32_u32(decay_done),
                               vnegq_f32(vrecpeq_f32(env->release_samples)),
                               env->increment);

    // -----------------------------------------------------------------
    // 4c. Release -> Off transition (for voices that finished release)
    // -----------------------------------------------------------------
    uint32x4_t release_done = vandq_u32(stage_done, release_stage);

    // Update stage to off for voices that finished release
    env->stage = vbslq_u32(release_done,
                           vdupq_n_u32(ENV_STATE_OFF),
                           env->stage);

    // Set level to 0 for voices that turned off
    env->level = vbslq_f32(vreinterpretq_f32_u32(release_done),
                           vdupq_n_f32(0.0f),
                           env->level);

    // Zero out increment for off voices
    env->increment = vbslq_f32(vreinterpretq_f32_u32(release_done),
                               vdupq_n_f32(0.0f),
                               env->increment);

    // -----------------------------------------------------------------
    // 5. Clamp level to [0, 1] for all voices
    // -----------------------------------------------------------------
    env->level = vmaxq_f32(vdupq_n_f32(0.0f),
                           vminq_f32(vdupq_n_f32(1.0f), env->level));
}

/**
 * Check if any voices are still active
 * @return Non-zero if at least one voice is active
 */
fast_inline uint32_t neon_envelope_active(neon_envelope_t* env) {
    uint32x4_t not_off = vcgtq_u32(vdupq_n_u32(ENV_STATE_OFF), env->stage);
    return vgetq_lane_u32(not_off, 0) |
           vgetq_lane_u32(not_off, 1) |
           vgetq_lane_u32(not_off, 2) |
           vgetq_lane_u32(not_off, 3);
}

/**
 * Get current envelope level for a specific voice
 */
fast_inline float neon_envelope_get_voice(const neon_envelope_t* env, uint32_t voice_idx) {
    return vgetq_lane_f32(env->level, voice_idx);
}

// ========== UNIT TEST ==========
#ifdef TEST_ENVELOPE

#include <stdio.h>
#include <assert.h>

void test_envelope_transitions() {
    neon_envelope_t env;
    neon_envelope_init(&env);

    // Trigger voice 0 only
    uint32x4_t mask = {1, 0, 0, 0};
    neon_envelope_trigger(&env, mask, 20);  // Use shape 20

    float last_level = 0.0f;
    int attack_samples = 0;
    int decay_samples = 0;
    int release_samples = 0;

    // Track through all stages
    while (neon_envelope_active(&env)) {
        neon_envelope_process(&env);
        float level = neon_envelope_get_voice(&env, 0);
        uint32_t stage = vgetq_lane_u32(env.stage, 0);

        switch (stage) {
            case ENV_STAGE_ATTACK:
                assert(level >= last_level);  // Should increase
                attack_samples++;
                break;
            case ENV_STAGE_DECAY:
                assert(level <= last_level);  // Should decrease
                decay_samples++;
                break;
            case ENV_STAGE_RELEASE:
                assert(level <= last_level);  // Should decrease
                release_samples++;
                break;
        }

        last_level = level;
    }

    printf("Attack samples: %d\n", attack_samples);
    printf("Decay samples: %d\n", decay_samples);
    printf("Release samples: %d\n", release_samples);
    printf("All stages completed: PASSED\n");
}

void test_parallel_processing() {
    neon_envelope_t env;
    neon_envelope_init(&env);

    // Trigger all 4 voices with different shapes
    uint32x4_t mask = {1, 1, 1, 1};
    neon_envelope_trigger(&env, mask, 0);   // All shape 0

    // Process 10 samples
    for (int i = 0; i < 10; i++) {
        neon_envelope_process(&env);

        // All voices should have same level (same shape)
        float l0 = neon_envelope_get_voice(&env, 0);
        float l1 = neon_envelope_get_voice(&env, 1);
        float l2 = neon_envelope_get_voice(&env, 2);
        float l3 = neon_envelope_get_voice(&env, 3);

        assert(l0 == l1 && l1 == l2 && l2 == l3);
    }

    printf("Parallel processing test: PASSED\n");
}

int main() {
    test_envelope_transitions();
    test_parallel_processing();
    return 0;
}

#endif // TEST_ENVELOPE