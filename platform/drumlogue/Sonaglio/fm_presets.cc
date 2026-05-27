/**
 * @file fm_presets.cc
 * @brief Factory preset data for Sonaglio FM percussion synth.
 *
 * HW test preset bank.
 *
 * The first four values after each preset name are:
 *   instrument, blend, gap, scatter
 *
 * Presets are deliberately named test1..test26 for the current hardware
 * evaluation phase. Once envelopes/output gain/engine balance are confirmed,
 * these can be replaced by musical factory presets.
 */

#include "fm_presets.h"
#include <stddef.h>

const fm_preset_t FM_PRESETS_TEST[NUM_OF_PRESETS] = {
    // test1
    {
        "test1",
        0, 50, 0, 0,
        80, 88, 20, 30,
        20, 25, 35, 45,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        80, 45, 65, 35
    },

    // test2
    {
        "test2",
        0, 55, 4, 8,
        90, 78, 20, 30,
        20, 25, 35, 45,
        0, 8, LFO_TARGET_PITCH, 8,
        0, 0, LFO_TARGET_NONE, 0,
        88, 60, 55, 45
    },

    // test3
    {
        "test3",
        1, 50, 0, 0,
        20, 40, 88, 72,
        20, 25, 35, 45,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        82, 65, 45, 35
    },

    // test4
    {
        "test4",
        1, 60, 8, 12,
        20, 40, 70, 90,
        20, 25, 35, 45,
        2, 20, LFO_TARGET_NOISE_MIX, 20,
        0, 0, LFO_TARGET_NONE, 0,
        92, 45, 60, 30
    },

    // test5
    {
        "test5",
        2, 50, 0, 0,
        20, 30, 20, 30,
        20, 25, 80, 88,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        86, 35, 75, 30
    },

    // test6
    {
        "test6",
        2, 45, 5, 8,
        20, 30, 20, 30,
        20, 25, 92, 55,
        1, 15, LFO_TARGET_INDEX, 15,
        0, 0, LFO_TARGET_NONE, 0,
        78, 60, 45, 40
    },

    // test7
    {
        "test7",
        3, 50, 0, 0,
        20, 30, 20, 30,
        85, 75, 20, 30,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        98, 45, 60, 35
    },

    // test8
    {
        "test8",
        3, 65, 15, 18,
        20, 30, 20, 30,
        70, 95, 20, 30,
        3, 15, LFO_TARGET_INDEX, 25,
        0, 0, LFO_TARGET_NONE, 0,
        104, 35, 70, 45
    },

    // test9
    {
        "test9",
        4, 50, 6, 8,
        80, 82, 78, 70,
        20, 25, 20, 30,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        84, 55, 60, 35
    },

    // test10
    {
        "test10",
        5, 45, 10, 10,
        82, 88, 20, 30,
        20, 25, 72, 85,
        0, 0, LFO_TARGET_NONE, 0,
        1, 0, LFO_TARGET_NONE, 0,
        88, 45, 70, 30
    },

    // test11
    {
        "test11",
        6, 40, 8, 12,
        85, 80, 20, 30,
        78, 78, 20, 30,
        1, 15, LFO_TARGET_INDEX, 15,
        0, 0, LFO_TARGET_NONE, 0,
        96, 50, 62, 38
    },

    // test12
    {
        "test12",
        7, 55, 8, 12,
        20, 30, 78, 75,
        20, 25, 72, 78,
        0, 0, LFO_TARGET_NONE, 0,
        2, 0, LFO_TARGET_NONE, 0,
        90, 55, 62, 32
    },

    // test13
    {
        "test13",
        8, 55, 10, 18,
        20, 30, 76, 70,
        78, 72, 20, 30,
        2, 18, LFO_TARGET_NOISE_MIX, 22,
        0, 0, LFO_TARGET_NONE, 0,
        100, 55, 55, 42
    },

    // test14
    {
        "test14",
        9, 55, 12, 20,
        20, 30, 20, 30,
        80, 82, 78, 70,
        1, 18, LFO_TARGET_INDEX, 18,
        3, 0, LFO_TARGET_NONE, 0,
        102, 50, 68, 40
    },

    // test15
    {
        "test15",
        4, 70, 22, 25,
        75, 80, 78, 75,
        20, 25, 20, 30,
        0, 10, LFO_TARGET_ENV, 10,
        0, 0, LFO_TARGET_NONE, 0,
        72, 50, 50, 25
    },

    // test16
    {
        "test16",
        5, 35, 25, 25,
        78, 90, 20, 30,
        20, 25, 70, 85,
        0, 12, LFO_TARGET_PITCH, 12,
        4, 0, LFO_TARGET_NONE, 0,
        74, 40, 75, 30
    },

    // test17
    {
        "test17",
        0, 70, 18, 20,
        65, 98, 20, 30,
        20, 25, 20, 30,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        48, 30, 85, 20
    },

    // test18
    {
        "test18",
        1, 35, 18, 25,
        20, 30, 95, 50,
        20, 25, 20, 30,
        0, 20, LFO_TARGET_NOISE_MIX, 35,
        0, 0, LFO_TARGET_NONE, 0,
        66, 75, 30, 55
    },

    // test19
    {
        "test19",
        3, 50, 25, 35,
        20, 30, 20, 30,
        95, 90, 20, 30,
        4, 25, LFO_TARGET_METAL_GATE, 40,
        0, 0, LFO_TARGET_NONE, 0,
        110, 30, 80, 50
    },

    // test20
    {
        "test20",
        2, 50, 20, 25,
        20, 30, 20, 30,
        20, 25, 65, 95,
        2, 10, LFO_TARGET_PITCH, 10,
        5, 0, LFO_TARGET_NONE, 0,
        54, 35, 90, 20
    },

    // test21
    {
        "test21",
        6, 65, 30, 40,
        70, 85, 20, 30,
        80, 90, 20, 30,
        0, 0, LFO_TARGET_NONE, 0,
        6, 10, LFO_TARGET_INDEX, 10,
        112, 40, 75, 35
    },

    // test22
    {
        "test22",
        9, 55, 35, 45,
        20, 30, 20, 30,
        75, 95, 80, 70,
        4, 20, LFO_TARGET_LFO2_PHASE, 20,
        7, 15, LFO_TARGET_PITCH, 10,
        118, 35, 80, 30
    },

    // test23
    {
        "test23",
        8, 50, 15, 35,
        20, 30, 80, 75,
        82, 88, 20, 30,
        0, 30, LFO_TARGET_NOISE_MIX, 45,
        0, 10, LFO_TARGET_INDEX, 15,
        106, 55, 58, 55
    },

    // test24
    {
        "test24",
        4, 50, 4, 0,
        90, 90, 80, 80,
        20, 20, 20, 20,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        28, 45, 60, 30
    },

    // test25
    {
        "test25",
        5, 50, 0, 0,
        80, 85, 20, 20,
        20, 20, 80, 85,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        95, 35, 70, 20
    },

    // test26
    {
        "test26",
        9, 50, 12, 30,
        20, 20, 20, 20,
        85, 85, 85, 85,
        3, 20, LFO_TARGET_INDEX, 25,
        8, 20, LFO_TARGET_METAL_GATE, 20,
        127, 45, 75, 45
    }
};

const fm_preset_t FM_PRESETS[NUM_OF_PRESETS] = {
    // 0: TightKick
    {
        "TightKick",
        0, 35, 5, 5,
        72, 82, 8, 18,
        8, 8, 15, 22,
        0, 10, LFO_TARGET_PITCH, 10,
        0, 0, LFO_TARGET_NONE, 0,
        12, 35, 15, 20},

    // 1: HeavyKick
    {
        "HeavyKick",
        0, 62, 8, 5,
        55, 92, 10, 25,
        15, 10, 15, 30,
        0, 15, LFO_TARGET_PITCH, 20,
        0, 0, LFO_TARGET_NONE, 0,
        18, 25, 45, 15},

    // 2: ClickKick
    {
        "ClickKick",
        0, 20, 3, 15,
        90, 55, 5, 15,
        25, 10, 35, 20,
        1, 18, LFO_TARGET_INDEX, 20,
        1, 0, LFO_TARGET_NONE, 0,
        8, 55, 10, 35},

    // 3: CrackSnare
    {
        "CrackSnare",
        1, 45, 4, 15,
        15, 22, 92, 78,
        10, 10, 20, 20,
        2, 35, LFO_TARGET_INDEX, 50,
        0, 10, LFO_TARGET_NOISE_MIX, 15,
        20, 55, 20, 20},

    // 4: BodySnare
    {
        "BodySnare",
        1, 65, 6, 10,
        25, 20, 65, 95,
        15, 10, 10, 10,
        0, 20, LFO_TARGET_PITCH, 15,
        0, 0, LFO_TARGET_NONE, 0,
        25, 30, 55, 10},

    // 5: GhostSnare
    {
        "GhostSnare",
        1, 30, 22, 35,
        15, 20, 60, 45,
        5, 10, 5, 5,
        0, 8, LFO_TARGET_ENV, 20,
        0, 0, LFO_TARGET_NONE, 0,
        16, 25, 10, 5},

    // 6: RimSnare
    {
        "RimSnare",
        1, 25, 6, 20,
        15, 10, 85, 45,
        10, 10, 20, 10,
        1, 28, LFO_TARGET_PITCH, -35,
        0, 15, LFO_TARGET_INDEX, 10,
        30, 45, 15, 20},

    // 7: MetalClang — bright cymbal clang with a real ring tail
    {
        "MetalClang",
        3, 50, 8, 12,
        10, 10, 20, 15,
        72, 46, 10, 10,
        0, 35, LFO_TARGET_INDEX, 30,
        0, 0, LFO_TARGET_NONE, 0,
        99, 64, 42, 38},  // rebalanced for hotter metallic engine: less peew, tighter clang

    // 8: MetalWash — sustained shimmering metallic wash
    {
        "MetalWash",
        3, 75, 35, 45,
        10, 15, 25, 20,
        62, 74, 20, 25,
        0, 20, LFO_TARGET_ENV, 40,
        0, 0, LFO_TARGET_NONE, 0,
        106, 44, 48, 26},  // rebalanced for denser ring to avoid over-bright wash

    // 9: GongHit — dark, dense, long-ringing gong
    {
        "GongHit",
        3, 85, 28, 35,
        0, 0, 0, 0,
        42, 82, 0, 0,
        3, 10, LFO_TARGET_PITCH, -20,
        0, 0, LFO_TARGET_NONE, 0,
        238, 22, 42, 18},  // rebalanced gong: keep long ring, reduce whistle dominance

    // 10: BellRing — bright cymbal-bell with shimmering long ring
    {
        "BellRing",
        3, 80, 45, 25,
        0, 0, 0, 0,
        58, 70, 0, 0,
        4, 18, LFO_TARGET_INDEX, 35,
        0, 0, LFO_TARGET_NONE, 0,
        110, 30, 48, 26  // rebalanced bell ring for updated metal FM density
    },

    // 11: PercBlock
    {
        "PercBlock",
        2, 35, 6, 8,
        20, 20, 10, 20,
        10, 15, 95, 65,
        0, 20, LFO_TARGET_PITCH, 15,
        0, 0, LFO_TARGET_NONE, 0,
        14, 55, 20, 20},

    // 12: PercTom
    {
        "PercTom",
        2, 70, 12, 10,
        15, 60, 10, 20,
        10, 10, 75, 90,
        0, 12, LFO_TARGET_ENV, 20,
        1, 0, LFO_TARGET_NONE, 0,
        22, 25, 50, 15},

    // 13: PercWood — short woody knock / clave-like attack
    {
        "PercWood",
        2, 28, 4, 8,
        22, 30, 8, 16,
        8, 8, 92, 32,
        1, 12, LFO_TARGET_INDEX, 8,
        0, 0, LFO_TARGET_NONE, 0,
        14, 42, 32, 8},

    // 14: DryHit
    {
        "DryHit",
        4, 50, 0, 0,
        60, 40, 55, 35,
        50, 35, 55, 35,
        0, 0, LFO_TARGET_NONE, 0,
        0, 0, LFO_TARGET_NONE, 0,
        4, 20, 20, 5},

    // 15: DriveKit
    {
        "DriveKit",
        4, 45, 4, 10,
        85, 65, 10, 15,
        15, 20, 20, 25,
        1, 22, LFO_TARGET_INDEX, 20,
        0, 0, LFO_TARGET_NONE, 0,
        10, 45, 25, 80},

    // 16: DarkPulse
    {
        "DarkPulse",
        5, 55, 18, 20,
        55, 45, 35, 45,
        35, 40, 35, 45,
        0, 8, LFO_TARGET_ENV, 10,
        0, 0, LFO_TARGET_NONE, 0,
        16, 20, 30, 20},

    // 17: BrightPulse
    {
        "BrightPulse",
        6, 55, 8, 20,
        70, 40, 60, 35,
        70, 55, 50, 35,
        1, 25, LFO_TARGET_INDEX, 25,
        0, 0, LFO_TARGET_NONE, 0,
        8, 55, 25, 15},

    // 18: Industrial
    {
        "Industrial",
        8, 65, 15, 35,
        30, 25, 65, 45,
        85, 70, 55, 40,
        0, 32, LFO_TARGET_METAL_GATE, 55,
        0, 20, LFO_TARGET_INDEX, 20,
        70, 65, 55, 75},  // env_shape was 128 + 70

    // 19: Shaker
    {
        "Shaker",
        9, 55, 10, 45,
        10, 10, 20, 20,
        75, 80, 85, 70,
        0, 55, LFO_TARGET_NOISE_MIX, 70,
        0, 30, LFO_TARGET_ENV, 20,
        12, 40, 45, 25},

    // 20: EuclidKit
    {
        "EuclidKit",
        5, 50, 20, 30,
        60, 55, 55, 55,
        50, 50, 50, 50,
        2, 20, LFO_TARGET_PITCH, 20,
        6, 15, LFO_TARGET_INDEX, 10,
        18, 45, 35, 20},

    // 21: WidePerc
    {
        "WidePerc",
        9, 45, 35, 50,
        25, 55, 20, 55,
        20, 40, 70, 80,
        1, 15, LFO_TARGET_LFO2_PHASE, 35,
        4, 20, LFO_TARGET_PITCH, 25,
        20, 35, 45, 15},

    // 22: LowBody
    {
        "LowBody",
        5, 45, 8, 10,
        65, 90, 20, 75,
        20, 25, 25, 65,
        0, 12, LFO_TARGET_PITCH, 10,
        0, 0, LFO_TARGET_NONE, 0,
        14, 20, 70, 10},

    // 23: HardDrive
    {
        "HardDrive",
        6, 55, 6, 25,
        90, 70, 70, 55,
        65, 55, 65, 55,
        1, 30, LFO_TARGET_INDEX, 30,
        0, 0, LFO_TARGET_NONE, 0,
        8, 60, 45, 95},

    // 24: SoftHit
    {
        "SoftHit",
        5, 45, 22, 20,
        30, 50, 30, 45,
        25, 35, 25, 45,
        0, 10, LFO_TARGET_ENV, 15,
        0, 0, LFO_TARGET_NONE, 0,
        4, 20, 35, 10},

    // 25: Experimental
    {
        "ExpTrack",
        9, 50, 45, 70,
        55, 55, 55, 55,
        55, 55, 55, 55,
        4, 40, LFO_TARGET_LFO2_PHASE, 50,
        8, 30, LFO_TARGET_METAL_GATE, 40,
        40, 50, 50, 50}};  // env_shape was 128 + 40


void load_fm_preset(uint8_t idx, int8_t *params) {
    if (idx >= NUM_OF_PRESETS || params == NULL) {
        return;
    }

    const fm_preset_t *p = &FM_PRESETS[idx];

    params[PARAM_INSTRUMENT] = p->instrument_sel;
    params[PARAM_BLEND]      = p->blend;
    params[PARAM_GAP]        = p->gap;
    params[PARAM_SCATTER]    = p->scatter;

    params[PARAM_KICK_ATK]   = p->kick_attack;
    params[PARAM_KICK_BODY]  = p->kick_body;
    params[PARAM_SNARE_ATK]  = p->snare_attack;
    params[PARAM_SNARE_BODY] = p->snare_body;

    params[PARAM_METAL_ATK]  = p->metal_attack;
    params[PARAM_METAL_BODY] = p->metal_body;
    params[PARAM_PERC_ATK]   = p->perc_attack;
    params[PARAM_PERC_BODY]  = p->perc_body;

    params[PARAM_LFO1_SHAPE]  = p->lfo1_shape;
    params[PARAM_LFO1_RATE]   = p->lfo1_rate;
    params[PARAM_LFO1_TARGET] = p->lfo1_target;
    params[PARAM_LFO1_DEPTH]  = p->lfo1_depth;

    params[PARAM_EUCL_TUN]    = p->eucl_tun;
    params[PARAM_LFO2_RATE]   = p->lfo2_rate;
    params[PARAM_LFO2_TARGET] = p->lfo2_target;
    params[PARAM_LFO2_DEPTH]  = p->lfo2_depth;

    params[PARAM_ENV_SHAPE]   = p->env_shape;
    params[PARAM_HIT_SHAPE]   = p->hit_shape;
    params[PARAM_BODY_TILT]   = p->body_tilt;
    params[PARAM_NOISE_CHAR]  = p->noise_char;
}
