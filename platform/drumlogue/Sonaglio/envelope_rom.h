
/**
 * @file envelope_rom.h
 * @brief 128 predefined ADR envelopes for percussion
 *
 * Fixed version with proper parallel processing for all 4 voices
 * Attack → Decay → Release (ADR) - no sustain
 */

#ifndef D633D6FD_1030_466A_92D2_2D9840E3CFA4
#define D633D6FD_1030_466A_92D2_2D9840E3CFA4

#include <arm_neon.h>
#include <stdint.h>
#include "constants.h"
#include "float_math.h"

// Envelope stages
#define ENV_STAGE_ATTACK  0
#define ENV_STAGE_DECAY   1
#define ENV_STAGE_RELEASE 2
#define ENV_STATE_OFF     3

// Envelope curve types
#define ENV_CURVE_LINEAR       0
#define ENV_CURVE_EXPONENTIAL  1
#define ENV_CURVE_LOG          2
#define ENV_CURVE_SIGMOID      3
#define ENV_CURVE_PUNCH        4

typedef struct {
    uint8_t attack_ms;    // 0-50ms
    uint16_t decay_ms;    // 20-500ms
    uint16_t release_ms;  // 10-500ms
    uint8_t curve_type;   // 0=linear, 1=exponential, 2=log, 3=sigmoid, 4=punch
} env_curve_t;

/** What I did not change

I did not rewrite the 128 envelope ROM yet.

The table is still broad, including tight, punchy, fat, boomy, gated, ambient, reverse-like, and pad-like ranges. For Sonaglio's new focused identity, we may later want a new curated ROM with more aggressive percussion shapes and fewer pad/reverse shapes, but I would do that only after confirming the generator is stable.

Recommendation

Use this revised runtime first. Then later, after listening, we can redesign the ROM categories around Sonaglio's current direction:

micro-click
tight drum
heavy body
metallic snap
flam/combo-friendly
experimental tails

The runtime fix should come before the ROM redesign */


// 128 predefined envelope curves (as previously defined)
static const env_curve_t ENV_ROM[128] = {
    // Sonaglio v3 curated envelopes.
    // Ranges are intentionally longer than the previous table because
    // first HW testing showed low indices were too short/nearly inaudible.
    // 0-15 tight, 16-31 punch, 32-47 body, 48-63 long body,
    // 64-79 gated/flam, 80-95 open linear, 96-111 metallic, 112-127 experimental.
    {0, 45, 20, 1},  // 0: Tight click/body
    {0, 55, 26, 1},  // 1: Tight click/body
    {0, 65, 32, 1},  // 2: Tight click/body
    {0, 75, 38, 1},  // 3: Tight click/body
    {1, 85, 44, 1},  // 4: Tight click/body
    {1, 95, 50, 1},  // 5: Tight click/body
    {1, 105, 56, 1},  // 6: Tight click/body
    {1, 115, 62, 1},  // 7: Tight click/body
    {1, 125, 68, 1},  // 8: Tight click/body
    {1, 135, 74, 1},  // 9: Tight click/body
    {2, 145, 80, 1},  // 10: Tight click/body
    {2, 155, 86, 1},  // 11: Tight click/body
    {2, 165, 92, 1},  // 12: Tight click/body
    {2, 175, 98, 1},  // 13: Tight click/body
    {2, 185, 104, 1},  // 14: Tight click/body
    {2, 195, 110, 1},  // 15: Tight click/body
    {1, 180, 90, 4},  // 16: Punch drum
    {1, 198, 100, 4},  // 17: Punch drum
    {1, 216, 110, 4},  // 18: Punch drum
    {1, 234, 120, 4},  // 19: Punch drum
    {2, 252, 130, 4},  // 20: Punch drum
    {2, 270, 140, 4},  // 21: Punch drum
    {2, 288, 150, 4},  // 22: Punch drum
    {2, 306, 160, 4},  // 23: Punch drum
    {3, 324, 170, 4},  // 24: Punch drum
    {3, 342, 180, 4},  // 25: Punch drum
    {3, 360, 190, 4},  // 26: Punch drum
    {3, 378, 200, 4},  // 27: Punch drum
    {4, 396, 210, 4},  // 28: Punch drum
    {4, 414, 220, 4},  // 29: Punch drum
    {4, 432, 230, 4},  // 30: Punch drum
    {4, 450, 240, 4},  // 31: Punch drum
    {2, 380, 180, 1},  // 32: Body drum
    {2, 405, 196, 1},  // 33: Body drum
    {2, 430, 212, 1},  // 34: Body drum
    {2, 455, 228, 1},  // 35: Body drum
    {2, 480, 244, 1},  // 36: Body drum
    {3, 505, 260, 1},  // 37: Body drum
    {3, 530, 276, 1},  // 38: Body drum
    {3, 555, 292, 1},  // 39: Body drum
    {3, 580, 308, 1},  // 40: Body drum
    {3, 605, 324, 1},  // 41: Body drum
    {4, 630, 340, 1},  // 42: Body drum
    {4, 655, 356, 1},  // 43: Body drum
    {4, 680, 372, 1},  // 44: Body drum
    {4, 705, 388, 1},  // 45: Body drum
    {4, 730, 404, 1},  // 46: Body drum
    {5, 755, 420, 1},  // 47: Body drum
    {3, 650, 280, 1},  // 48: Long body
    {3, 685, 300, 1},  // 49: Long body
    {3, 720, 320, 1},  // 50: Long body
    {3, 755, 340, 1},  // 51: Long body
    {4, 790, 360, 1},  // 52: Long body
    {4, 825, 380, 1},  // 53: Long body
    {4, 860, 400, 1},  // 54: Long body
    {4, 895, 420, 1},  // 55: Long body
    {5, 930, 440, 1},  // 56: Long body
    {5, 965, 460, 1},  // 57: Long body
    {5, 1000, 480, 1},  // 58: Long body
    {5, 1035, 500, 1},  // 59: Long body
    {6, 1070, 520, 1},  // 60: Long body
    {6, 1105, 540, 1},  // 61: Long body
    {6, 1140, 560, 1},  // 62: Long body
    {6, 1175, 580, 1},  // 63: Long body
    {0, 120, 300, 1},  // 64: Gate/flam
    {0, 138, 330, 1},  // 65: Gate/flam
    {0, 156, 360, 1},  // 66: Gate/flam
    {0, 174, 390, 1},  // 67: Gate/flam
    {0, 192, 420, 1},  // 68: Gate/flam
    {0, 210, 450, 1},  // 69: Gate/flam
    {1, 228, 480, 1},  // 70: Gate/flam
    {1, 246, 510, 1},  // 71: Gate/flam
    {1, 264, 540, 1},  // 72: Gate/flam
    {1, 282, 570, 1},  // 73: Gate/flam
    {1, 300, 600, 1},  // 74: Gate/flam
    {1, 318, 630, 1},  // 75: Gate/flam
    {2, 336, 660, 1},  // 76: Gate/flam
    {2, 354, 690, 1},  // 77: Gate/flam
    {2, 372, 720, 1},  // 78: Gate/flam
    {2, 390, 750, 1},  // 79: Gate/flam
    {4, 500, 420, 0},  // 80: Open linear
    {4, 545, 445, 0},  // 81: Open linear
    {4, 590, 470, 0},  // 82: Open linear
    {5, 635, 495, 0},  // 83: Open linear
    {5, 680, 520, 0},  // 84: Open linear
    {5, 725, 545, 0},  // 85: Open linear
    {6, 770, 570, 0},  // 86: Open linear
    {6, 815, 595, 0},  // 87: Open linear
    {6, 860, 620, 0},  // 88: Open linear
    {7, 905, 645, 0},  // 89: Open linear
    {7, 950, 670, 0},  // 90: Open linear
    {7, 995, 695, 0},  // 91: Open linear
    {8, 1040, 720, 0},  // 92: Open linear
    {8, 1085, 745, 0},  // 93: Open linear
    {8, 1130, 770, 0},  // 94: Open linear
    {9, 1175, 795, 0},  // 95: Open linear
    {0, 350, 200, 3},  // 96: Metallic tail
    {0, 405, 225, 3},  // 97: Metallic tail
    {0, 460, 250, 3},  // 98: Metallic tail
    {0, 515, 275, 3},  // 99: Metallic tail
    {0, 570, 300, 3},  // 100: Metallic tail
    {0, 625, 325, 3},  // 101: Metallic tail
    {1, 680, 350, 3},  // 102: Metallic tail
    {1, 735, 375, 3},  // 103: Metallic tail
    {1, 790, 400, 3},  // 104: Metallic tail
    {1, 845, 425, 3},  // 105: Metallic tail
    {1, 900, 450, 3},  // 106: Metallic tail
    {1, 955, 475, 3},  // 107: Metallic tail
    {1, 1010, 500, 3},  // 108: Metallic tail
    {1, 1065, 525, 3},  // 109: Metallic tail
    {1, 1120, 550, 3},  // 110: Metallic tail
    {1, 1175, 575, 3},  // 111: Metallic tail
    {2, 800, 500, 0},  // 112: Experimental long
    {2, 870, 535, 0},  // 113: Experimental long
    {2, 940, 570, 0},  // 114: Experimental long
    {2, 1010, 605, 0},  // 115: Experimental long
    {3, 1080, 640, 0},  // 116: Experimental long
    {3, 1150, 675, 0},  // 117: Experimental long
    {3, 1220, 710, 0},  // 118: Experimental long
    {3, 1290, 745, 0},  // 119: Experimental long
    {4, 1360, 780, 0},  // 120: Experimental long
    {4, 1430, 815, 0},  // 121: Experimental long
    {4, 1500, 850, 0},  // 122: Experimental long
    {4, 1570, 885, 0},  // 123: Experimental long
    {5, 1640, 920, 0},  // 124: Experimental long
    {5, 1710, 955, 0},  // 125: Experimental long
    {5, 1780, 990, 0},  // 126: Experimental long
    {5, 1850, 1025, 0}  // 127: Experimental long
};

// previous values, before Sonaglio v3 redesign and runtime fix. Still here for reference and testing, but will likely be removed after confirming the new ROM and runtime are solid.
static const env_curve_t OLD_ENV_ROM[128] = {
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
    {4, 180, 90, 4},   // 16
    {4, 190, 95, 4},   // 17
    {5, 200, 100, 4},  // 18
    {5, 210, 105, 4},  // 19
    {6, 220, 110, 4},  // 20
    {6, 230, 115, 4},  // 21
    {7, 240, 120, 4},  // 22
    {7, 250, 125, 4},  // 23
    {8, 260, 130, 4},  // 24
    {8, 270, 135, 4},  // 25
    {9, 280, 140, 4},  // 26
    {9, 290, 145, 4},  // 27
    {10, 300, 150, 4}, // 28
    {10, 310, 155, 4}, // 29
    {11, 320, 160, 4}, // 30
    {11, 330, 165, 4}, // 31

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
 * Expand a per-lane boolean mask to a proper NEON bit-select mask.
 *
 * vbslq_* expects lanes to be either 0x00000000 or 0xFFFFFFFF.
 * Some tests and call sites may pass 0/1 style masks; this helper makes
 * both forms safe.
 */
fast_inline uint32x4_t envelope_full_mask(uint32x4_t mask) {
    return vcgtq_u32(mask, vdupq_n_u32(0));
}

/**
 * Get envelope parameters from ROM.
 *
 * decay_ms and release_ms are uint16_t in the ROM and must not be truncated.
 */
fast_inline void get_envelope(uint8_t shape_idx,
                              uint8_t* attack_ms,
                              uint16_t* decay_ms,
                              uint16_t* release_ms,
                              uint8_t* curve_type) {
    shape_idx &= 0x7F;
    const env_curve_t* env = &ENV_ROM[shape_idx];
    // Runtime curation for Sonaglio percussion: keep onsets fast and tails
    // controlled even when ROM entries are broad/experimental.
    uint8_t a = env->attack_ms;
    uint16_t d = env->decay_ms;
    uint16_t r = env->release_ms;
    // Keep onsets fast, but allow long metallic/experimental tails to ring.
    // The metallic ROM range (96-127) is designed for 350-1850ms decays; the
    // metal engine's sqrt amplitude envelope needs them to breathe.
    if (a > 6) a = 6;
    if (d > 1200) d = 1200;
    if (r > 700) r = 700;
    *attack_ms = a;
    *decay_ms = d;
    *release_ms = r;
    *curve_type = env->curve_type;
}

/**
 * Convert milliseconds to samples at 48 kHz.
 */
fast_inline float envelope_ms_to_samples_u8(uint8_t ms) {
    float s = (float)ms * 48.0f;
    return (s < 1.0f) ? 1.0f : s;
}

fast_inline float envelope_ms_to_samples_u16(uint16_t ms) {
    float s = (float)ms * 48.0f;
    return (s < 1.0f) ? 1.0f : s;
}

/**
 * Build the per-stage increment / coefficient.
 *
 * For linear stages:
 *   attack  : +1/N
 *   decay   : -1/N
 *   release : set in neon_envelope_release() from current level
 *
 * For exponential stages this is a one-pole coefficient:
 *   attack  : level += (1 - level) * coeff
 *   decay   : level -= level * coeff
 *   release : level -= level * coeff
 *
 * This is deliberately approximate and cheap. Stage completion is still driven
 * by samples_left, so timing stays deterministic.
 */
fast_inline float envelope_stage_coeff(float samples, uint32_t stage, uint32_t curve) {
    if (samples < 1.0f) samples = 1.0f;

    if (curve == ENV_CURVE_EXPONENTIAL || curve == ENV_CURVE_LOG ||
        curve == ENV_CURVE_SIGMOID || curve == ENV_CURVE_PUNCH) {
        float a = 4.0f, d = 5.0f, r = 6.0f;
        if (curve == ENV_CURVE_LOG) {
            a = 3.2f; d = 4.2f; r = 4.8f;
        } else if (curve == ENV_CURVE_SIGMOID) {
            a = 4.8f; d = 4.0f; r = 5.0f;
        } else if (curve == ENV_CURVE_PUNCH) {
            a = 6.2f; d = 7.2f; r = 7.0f;
        }
        switch (stage) {
            case ENV_STAGE_ATTACK:  return a / samples;
            case ENV_STAGE_DECAY:   return d / samples;
            case ENV_STAGE_RELEASE: return r / samples;
            default:                return 0.0f;
        }
    }

    switch (stage) {
        case ENV_STAGE_ATTACK:  return  1.0f / samples;
        case ENV_STAGE_DECAY:   return -1.0f / samples;
        case ENV_STAGE_RELEASE: return -1.0f / samples;
        default:                return  0.0f;
    }
}

/**
 * Trigger envelope for selected voices.
 *
 * @param env Envelope state
 * @param voice_mask Which voices to trigger. Accepts either full-bit masks
 *                   (0xFFFFFFFF per active lane) or simple 0/1 masks.
 * @param shape_idx Envelope shape index (0-127)
 */
fast_inline void neon_envelope_trigger(neon_envelope_t* env,
                                       uint32x4_t voice_mask,
                                       uint8_t shape_idx) {
    voice_mask = envelope_full_mask(voice_mask);

    uint8_t a_ms, curve;
    uint16_t d_ms, r_ms;
    get_envelope(shape_idx, &a_ms, &d_ms, &r_ms, &curve);

    float attack_samps  = envelope_ms_to_samples_u8(a_ms);
    float decay_samps   = envelope_ms_to_samples_u16(d_ms);
    float release_samps = envelope_ms_to_samples_u16(r_ms);

    float attack_coeff = envelope_stage_coeff(attack_samps,
                                              ENV_STAGE_ATTACK,
                                              curve);

    env->attack_samples = vbslq_f32(voice_mask,
                                    vdupq_n_f32(attack_samps),
                                    env->attack_samples);
    env->decay_samples = vbslq_f32(voice_mask,
                                   vdupq_n_f32(decay_samps),
                                   env->decay_samples);
    env->release_samples = vbslq_f32(voice_mask,
                                     vdupq_n_f32(release_samps),
                                     env->release_samples);

    env->curve_type = vbslq_u32(voice_mask,
                                vdupq_n_u32(curve),
                                env->curve_type);

    env->stage = vbslq_u32(voice_mask,
                           vdupq_n_u32(ENV_STAGE_ATTACK),
                           env->stage);

    env->samples_left = vbslq_u32(voice_mask,
                                  vcvtq_u32_f32(vdupq_n_f32(attack_samps)),
                                  env->samples_left);

    env->increment = vbslq_f32(voice_mask,
                               vdupq_n_f32(attack_coeff),
                               env->increment);

    env->level = vbslq_f32(voice_mask,
                           vdupq_n_f32(0.0f),
                           env->level);
}

/**
 * Process one sample for all 4 voices in parallel.
 *
 * This version fixes three issues in the older implementation:
 * - OFF lanes no longer underflow samples_left.
 * - curve_type is actually used.
 * - 0/1 voice masks are supported by trigger/release helpers.
 */
fast_inline void neon_envelope_process(neon_envelope_t* env) {
    const uint32x4_t off_stage = vdupq_n_u32(ENV_STATE_OFF);
    const uint32x4_t one_u32   = vdupq_n_u32(1);

    const float32x4_t zero = vdupq_n_f32(0.0f);
    const float32x4_t one  = vdupq_n_f32(1.0f);

    uint32x4_t active = vcgtq_u32(off_stage, env->stage);

    uint32x4_t attack_stage  = vceqq_u32(env->stage, vdupq_n_u32(ENV_STAGE_ATTACK));
    uint32x4_t decay_stage   = vceqq_u32(env->stage, vdupq_n_u32(ENV_STAGE_DECAY));
    uint32x4_t release_stage = vceqq_u32(env->stage, vdupq_n_u32(ENV_STAGE_RELEASE));
    uint32x4_t exp_curve     = vceqq_u32(env->curve_type, vdupq_n_u32(ENV_CURVE_EXPONENTIAL));
    uint32x4_t log_curve     = vceqq_u32(env->curve_type, vdupq_n_u32(ENV_CURVE_LOG));
    uint32x4_t sigmoid_curve = vceqq_u32(env->curve_type, vdupq_n_u32(ENV_CURVE_SIGMOID));
    uint32x4_t punch_curve   = vceqq_u32(env->curve_type, vdupq_n_u32(ENV_CURVE_PUNCH));

    // Stage done before decrement. Treat samples_left <= 1 as done.
    uint32x4_t done = vandq_u32(active, vcgeq_u32(one_u32, env->samples_left));

    // Linear update: level += increment
    float32x4_t linear_level = vaddq_f32(env->level, env->increment);

    // Exponential-ish update:
    // attack  : level += (1 - level) * coeff
    // decay   : level -= level * coeff
    // release : level -= level * coeff
    float32x4_t attack_exp = vaddq_f32(env->level,
                                       vmulq_f32(vsubq_f32(one, env->level),
                                                 env->increment));
    float32x4_t fall_exp = vsubq_f32(env->level,
                                     vmulq_f32(env->level, env->increment));

    float32x4_t exp_level = vbslq_f32(attack_stage, attack_exp, fall_exp);
    float32x4_t log_attack = vaddq_f32(env->level,
                                       vmulq_f32(vsubq_f32(one, env->level), vmulq_n_f32(env->increment, 0.72f)));
    float32x4_t log_fall = vsubq_f32(env->level,
                                     vmulq_f32(neon_sqrtq_f32(vmaxq_f32(env->level, zero)), vmulq_n_f32(env->increment, 0.72f)));
    float32x4_t log_level = vbslq_f32(attack_stage, log_attack, log_fall);

    float32x4_t sig_attack = vaddq_f32(env->level,
                                       vmulq_f32(vsubq_f32(one, env->level), vmulq_n_f32(env->increment, 0.90f)));
    float32x4_t sig_fall = vsubq_f32(env->level,
                                     vmulq_f32(env->level, vmulq_n_f32(env->increment, 0.90f)));
    float32x4_t sigmoid_level = vbslq_f32(attack_stage, sig_attack, sig_fall);

    float32x4_t punch_attack = vaddq_f32(env->level,
                                         vmulq_f32(vsubq_f32(one, env->level), vmulq_n_f32(env->increment, 1.35f)));
    float32x4_t punch_fall = vsubq_f32(env->level,
                                       vmulq_f32(env->level, vmulq_n_f32(env->increment, 1.25f)));
    float32x4_t punch_level = vbslq_f32(attack_stage, punch_attack, punch_fall);

    float32x4_t next_level = linear_level;
    next_level = vbslq_f32(exp_curve, exp_level, next_level);
    next_level = vbslq_f32(log_curve, log_level, next_level);
    next_level = vbslq_f32(sigmoid_curve, sigmoid_level, next_level);
    next_level = vbslq_f32(punch_curve, punch_level, next_level);

    env->level = vbslq_f32(active, next_level, env->level);

    // Decrement samples_left only for active, non-done lanes.
    uint32x4_t decrement_mask = vandq_u32(active, vmvnq_u32(done));
    env->samples_left = vbslq_u32(decrement_mask,
                                  vsubq_u32(env->samples_left, one_u32),
                                  env->samples_left);

    // -----------------------------------------------------------------
    // Stage transitions
    // -----------------------------------------------------------------

    uint32x4_t attack_done = vandq_u32(done, attack_stage);

    env->stage = vbslq_u32(attack_done,
                           vdupq_n_u32(ENV_STAGE_DECAY),
                           env->stage);

    env->samples_left = vbslq_u32(attack_done,
                                  vcvtq_u32_f32(env->decay_samples),
                                  env->samples_left);

    // Force exact peak at the end of attack.
    env->level = vbslq_f32(attack_done, one, env->level);

    // Decay coefficient/increment.
    // For linear: -1/decay_samples.
    // For nonlinear curves: positive one-pole coefficients per curve family.
    float32x4_t rec_decay = vrecpeq_f32(env->decay_samples);
    rec_decay = vmulq_f32(rec_decay, vrecpsq_f32(env->decay_samples, rec_decay));

    float32x4_t linear_decay_inc = vnegq_f32(rec_decay);
    float32x4_t decay_coeff = vmulq_n_f32(rec_decay, 5.0f);
    decay_coeff = vbslq_f32(log_curve, vmulq_n_f32(rec_decay, 4.2f), decay_coeff);
    decay_coeff = vbslq_f32(sigmoid_curve, vmulq_n_f32(rec_decay, 4.0f), decay_coeff);
    decay_coeff = vbslq_f32(punch_curve, vmulq_n_f32(rec_decay, 7.2f), decay_coeff);
    uint32x4_t nonlinear_curve = vorrq_u32(vorrq_u32(exp_curve, log_curve),
                                           vorrq_u32(sigmoid_curve, punch_curve));
    float32x4_t decay_inc = vbslq_f32(nonlinear_curve,
                                      decay_coeff,
                                      linear_decay_inc);

    env->increment = vbslq_f32(attack_done, decay_inc, env->increment);

    uint32x4_t decay_done = vandq_u32(done, decay_stage);

    env->stage = vbslq_u32(decay_done,
                           vdupq_n_u32(ENV_STAGE_RELEASE),
                           env->stage);

    env->samples_left = vbslq_u32(decay_done,
                                  vcvtq_u32_f32(env->release_samples),
                                  env->samples_left);

    // Release coefficient/increment.
    // Linear release must start from current level to avoid jumps and reach zero.
    float32x4_t rec_release = vrecpeq_f32(env->release_samples);
    rec_release = vmulq_f32(rec_release, vrecpsq_f32(env->release_samples, rec_release));

    float32x4_t linear_release_inc = vnegq_f32(vmulq_f32(env->level, rec_release));
    float32x4_t release_coeff = vmulq_n_f32(rec_release, 6.0f);
    release_coeff = vbslq_f32(log_curve, vmulq_n_f32(rec_release, 4.8f), release_coeff);
    release_coeff = vbslq_f32(sigmoid_curve, vmulq_n_f32(rec_release, 5.0f), release_coeff);
    release_coeff = vbslq_f32(punch_curve, vmulq_n_f32(rec_release, 7.0f), release_coeff);
    float32x4_t release_inc = vbslq_f32(nonlinear_curve,
                                        release_coeff,
                                        linear_release_inc);

    env->increment = vbslq_f32(decay_done, release_inc, env->increment);

    uint32x4_t release_done = vandq_u32(done, release_stage);

    env->stage = vbslq_u32(release_done,
                           off_stage,
                           env->stage);

    env->samples_left = vbslq_u32(release_done,
                                  vdupq_n_u32(0),
                                  env->samples_left);

    env->level = vbslq_f32(release_done, zero, env->level);
    env->increment = vbslq_f32(release_done, zero, env->increment);

    // Clamp level to [0, 1].
    env->level = vmaxq_f32(zero, vminq_f32(one, env->level));
}

/**
 * Trigger release stage for selected voices (MIDI note-off).
 * Transitions from any active stage directly to release, starting from
 * the current level so there is no level jump.
 */
fast_inline void neon_envelope_release(neon_envelope_t* env,
                                       uint32x4_t voice_mask) {
    voice_mask = envelope_full_mask(voice_mask);

    uint32x4_t is_off = vceqq_u32(env->stage, vdupq_n_u32(ENV_STATE_OFF));
    uint32x4_t trigger = vandq_u32(voice_mask, vmvnq_u32(is_off));

    env->stage = vbslq_u32(trigger, vdupq_n_u32(ENV_STAGE_RELEASE), env->stage);

    env->samples_left = vbslq_u32(trigger,
                                  vcvtq_u32_f32(env->release_samples),
                                  env->samples_left);

    uint32x4_t exp_curve     = vceqq_u32(env->curve_type, vdupq_n_u32(ENV_CURVE_EXPONENTIAL));
    uint32x4_t log_curve     = vceqq_u32(env->curve_type, vdupq_n_u32(ENV_CURVE_LOG));
    uint32x4_t sigmoid_curve = vceqq_u32(env->curve_type, vdupq_n_u32(ENV_CURVE_SIGMOID));
    uint32x4_t punch_curve   = vceqq_u32(env->curve_type, vdupq_n_u32(ENV_CURVE_PUNCH));
    uint32x4_t nonlinear_curve = vorrq_u32(vorrq_u32(exp_curve, log_curve),
                                           vorrq_u32(sigmoid_curve, punch_curve));

    float32x4_t rec = vrecpeq_f32(env->release_samples);
    rec = vmulq_f32(rec, vrecpsq_f32(env->release_samples, rec));

    float32x4_t linear_release_inc = vnegq_f32(vmulq_f32(env->level, rec));
    float32x4_t release_coeff = vmulq_n_f32(rec, 6.0f);
    release_coeff = vbslq_f32(log_curve, vmulq_n_f32(rec, 4.8f), release_coeff);
    release_coeff = vbslq_f32(sigmoid_curve, vmulq_n_f32(rec, 5.0f), release_coeff);
    release_coeff = vbslq_f32(punch_curve, vmulq_n_f32(rec, 7.0f), release_coeff);
    float32x4_t release_inc = vbslq_f32(nonlinear_curve,
                                        release_coeff,
                                        linear_release_inc);

    env->increment = vbslq_f32(trigger, release_inc, env->increment);
}

/**
 * Check if any voices are still active.
 * @return Non-zero if at least one voice is active.
 */
fast_inline uint32_t neon_envelope_active(neon_envelope_t* env) {
    uint32x4_t not_off = vcgtq_u32(vdupq_n_u32(ENV_STATE_OFF), env->stage);
    return vgetq_lane_u32(not_off, 0) |
           vgetq_lane_u32(not_off, 1) |
           vgetq_lane_u32(not_off, 2) |
           vgetq_lane_u32(not_off, 3);
}

/**
 * Get current envelope level for a specific voice.
 */
fast_inline float neon_envelope_get_voice(const neon_envelope_t* env, uint32_t voice_idx) {
    switch (voice_idx & 3U) {
        case 0: return vgetq_lane_f32(env->level, 0);
        case 1: return vgetq_lane_f32(env->level, 1);
        case 2: return vgetq_lane_f32(env->level, 2);
        default: return vgetq_lane_f32(env->level, 3);
    }
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


#endif /* D633D6FD_1030_466A_92D2_2D9840E3CFA4 */
