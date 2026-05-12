/**
 * @file fm_presets.cc
 * @brief Factory preset data for FM Percussion Synth
 *
 * Presets are rebuilt around the selector-based Sonaglio model.
 * The four values after each preset name are now:
 *   instrument, blend, gap, scatter
 * The struct field names still use their historical prob_* labels for ABI compatibility.
 */

#include "fm_presets.h"
#include <stddef.h>

const fm_preset_t FM_PRESETS[NUM_OF_PRESETS] = {
    // 0: TightKick
    {
        "TightKick",
        0, 35, 5, 5,
        85, 75, 10, 20,
        20, 15, 20, 25,
        0, 10, LFO_TARGET_PITCH, 10,
        0, 0, LFO_TARGET_NONE, 0,
        12, 35, 15, 20},

    // 1: HeavyKick
    {
        "HeavyKick",
        0, 60, 8, 5,
        70, 95, 10, 30,
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
        20, 30, 90, 75,
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

    // 7: MetalClang
    {
        "MetalClang",
        3, 50, 8, 12,
        10, 10, 20, 15,
        95, 65, 10, 10,
        0, 35, LFO_TARGET_INDEX, 40,
        0, 0, LFO_TARGET_NONE, 0,
        8, 80, 35, 45},

    // 8: MetalWash
    {
        "MetalWash",
        3, 75, 35, 45,
        10, 15, 25, 20,
        80, 90, 20, 25,
        0, 20, LFO_TARGET_ENV, 40,
        0, 0, LFO_TARGET_NONE, 0,
        70, 55, 40, 30},

    // 9: GongHit
    {
        "GongHit",
        3, 85, 28, 35,
        0, 0, 0, 0,
        100, 95, 0, 0,
        3, 10, LFO_TARGET_PITCH, -20,
        0, 0, LFO_TARGET_NONE, 0,
        90, 25, 45, 20},  // env_shape was 128 + 90

    // 10: BellRing
    {
        "BellRing",
        3, 80, 45, 25,
        0, 0, 0, 0,
        100, 85, 0, 0,
        4, 18, LFO_TARGET_INDEX, 35,
        0, 0, LFO_TARGET_NONE, 0,
        110, 35, 55, 30  // env_shape was 128 + 110
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

    // 13: PercWood
    {
        "PercWood",
        2, 30, 5, 10,
        35, 45, 10, 20,
        10, 10, 85, 40,
        1, 15, LFO_TARGET_INDEX, 10,
        0, 0, LFO_TARGET_NONE, 0,
        18, 40, 35, 10},

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
    params[PARAM_DRIVE]       = p->drive;
}
