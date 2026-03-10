/**
 * @file fm_presets.h
 * @brief Factory presets for FM Percussion Synth
 *
 * Now includes 4 resonant synthesis presets
 */

#pragma once

#include <stdint.h>
#include "constants.h"
#include "resonant_synthesis.h"

// LFO target values (from lfo_enhanced.h)
#define LFO_TARGET_NONE     0
#define LFO_TARGET_PITCH    1
#define LFO_TARGET_INDEX    2
#define LFO_TARGET_ENV      3
#define LFO_TARGET_LFO2_PHASE 4
#define LFO_TARGET_LFO1_PHASE 5

// Engine modes
#define ENGINE_KICK     0
#define ENGINE_SNARE    1
#define ENGINE_METAL    2
#define ENGINE_PERC     3
#define ENGINE_RESONANT 4

typedef struct {
    char name[12];

    // Page 1: Probabilities
    uint8_t prob_kick;
    uint8_t prob_snare;
    uint8_t prob_metal;
    uint8_t prob_perc;

    // Page 2: Kick + Snare
    uint8_t kick_sweep;
    uint8_t kick_decay;
    uint8_t snare_noise;
    uint8_t snare_body;

    // Page 3: Metal + Perc
    uint8_t metal_inharm;
    uint8_t metal_bright;
    uint8_t perc_ratio;
    uint8_t perc_var;

    // Page 4: LFO1
    uint8_t lfo1_shape;     // 0-8
    uint8_t lfo1_rate;      // 0-100
    uint8_t lfo1_target;    // 0-5
    int8_t  lfo1_depth;     // -100 to 100

    // Page 5: LFO2
    uint8_t lfo2_shape;     // 0-8
    uint8_t lfo2_rate;      // 0-100
    uint8_t lfo2_target;    // 0-5
    int8_t  lfo2_depth;     // -100 to 100

    // Page 6: Envelope
    uint8_t env_shape;      // 0-127

    // NEW: Resonant parameters (using params 21-23)
    uint8_t resonant_mode;   // 0-4 (LP, BP, HP, Notch, Peak)
    uint8_t resonant_res;    // 0-100
    uint8_t resonant_center; // 0-100 (maps to 50-8000 Hz)
    uint8_t engine_map[4];   // Which engine each voice uses
} fm_preset_t;

// Factory presets (now with 4 resonant examples)
static const fm_preset_t FM_PRESETS[12] = {
    // Preset 0: "Deep Tribal" (original)
    {
        .name = "Deep Tribal",
        .prob_kick = 100, .prob_snare = 80, .prob_metal = 60, .prob_perc = 70,
        .kick_sweep = 80, .kick_decay = 60,
        .snare_noise = 30, .snare_body = 70,
        .metal_inharm = 40, .metal_bright = 50,
        .perc_ratio = 50, .perc_var = 40,
        .lfo1_shape = 0, .lfo1_rate = 20, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 30,
        .lfo2_shape = 1, .lfo2_rate = 5, .lfo2_target = LFO_TARGET_INDEX, .lfo2_depth = 20,
        .env_shape = 20,
        .resonant_mode = RESONANT_MODE_BANDPASS,
        .resonant_res = 50,
        .resonant_center = 30,  // ~2400 Hz
        .engine_map = {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 1: "Metal Storm" (original)
    {
        .name = "Metal Storm",
        .prob_kick = 60, .prob_snare = 80, .prob_metal = 100, .prob_perc = 50,
        .kick_sweep = 40, .kick_decay = 30,
        .snare_noise = 60, .snare_body = 40,
        .metal_inharm = 80, .metal_bright = 90,
        .perc_ratio = 30, .perc_var = 60,
        .lfo1_shape = 6, .lfo1_rate = 60, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 20,
        .lfo2_shape = 4, .lfo2_rate = 80, .lfo2_target = LFO_TARGET_LFO1_PHASE, .lfo2_depth = 50,
        .env_shape = 5,
        .resonant_mode = RESONANT_MODE_PEAK,
        .resonant_res = 70,
        .resonant_center = 60,  // ~4800 Hz
        .engine_map = {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 2: "Chordal Perc" (original)
    {
        .name = "Chordal Perc",
        .prob_kick = 70, .prob_snare = 70, .prob_metal = 70, .prob_perc = 70,
        .kick_sweep = 50, .kick_decay = 50,
        .snare_noise = 40, .snare_body = 50,
        .metal_inharm = 30, .metal_bright = 40,
        .perc_ratio = 60, .perc_var = 50,
        .lfo1_shape = 2, .lfo1_rate = 30, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 40,
        .lfo2_shape = 2, .lfo2_rate = 15, .lfo2_target = LFO_TARGET_PITCH, .lfo2_depth = -30,
        .env_shape = 40,
        .resonant_mode = RESONANT_MODE_LOWPASS,
        .resonant_res = 30,
        .resonant_center = 20,  // ~1600 Hz
        .engine_map = {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 3: "Phase Dance" (original)
    {
        .name = "Phase Dance",
        .prob_kick = 80, .prob_snare = 60, .prob_metal = 80, .prob_perc = 60,
        .kick_sweep = 30, .kick_decay = 40,
        .snare_noise = 50, .snare_body = 60,
        .metal_inharm = 50, .metal_bright = 60,
        .perc_ratio = 40, .perc_var = 30,
        .lfo1_shape = 3, .lfo1_rate = 45, .lfo1_target = LFO_TARGET_LFO2_PHASE, .lfo1_depth = 70,
        .lfo2_shape = 5, .lfo2_rate = 30, .lfo2_target = LFO_TARGET_LFO1_PHASE, .lfo2_depth = 70,
        .env_shape = 30,
        .resonant_mode = RESONANT_MODE_HIGHPASS,
        .resonant_res = 40,
        .resonant_center = 40,  // ~3200 Hz
        .engine_map = {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 4: "Bipolar Bass" (original)
    {
        .name = "Bipolar Bass",
        .prob_kick = 100, .prob_snare = 40, .prob_metal = 30, .prob_perc = 50,
        .kick_sweep = 90, .kick_decay = 80,
        .snare_noise = 20, .snare_body = 30,
        .metal_inharm = 20, .metal_bright = 30,
        .perc_ratio = 30, .perc_var = 20,
        .lfo1_shape = 1, .lfo1_rate = 10, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = -50,
        .lfo2_shape = 0, .lfo2_rate = 25, .lfo2_target = LFO_TARGET_NONE, .lfo2_depth = 0,
        .env_shape = 10,
        .resonant_mode = RESONANT_MODE_LOWPASS,
        .resonant_res = 20,
        .resonant_center = 10,  // ~800 Hz
        .engine_map = {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 5: "Snare Roll" (original)
    {
        .name = "Snare Roll",
        .prob_kick = 30, .prob_snare = 100, .prob_metal = 40, .prob_perc = 30,
        .kick_sweep = 20, .kick_decay = 30,
        .snare_noise = 80, .snare_body = 60,
        .metal_inharm = 30, .metal_bright = 40,
        .perc_ratio = 40, .perc_var = 30,
        .lfo1_shape = 1, .lfo1_rate = 90, .lfo1_target = LFO_TARGET_INDEX, .lfo1_depth = 60,
        .lfo2_shape = 1, .lfo2_rate = 45, .lfo2_target = LFO_TARGET_PITCH, .lfo2_depth = 20,
        .env_shape = 15,
        .resonant_mode = RESONANT_MODE_PEAK,
        .resonant_res = 60,
        .resonant_center = 70,  // ~5600 Hz
        .engine_map = {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 6: "Ambient Metals" (original)
    {
        .name = "Ambient Mtl",
        .prob_kick = 40, .prob_snare = 50, .prob_metal = 100, .prob_perc = 60,
        .kick_sweep = 30, .kick_decay = 70,
        .snare_noise = 40, .snare_body = 50,
        .metal_inharm = 70, .metal_bright = 80,
        .perc_ratio = 50, .perc_var = 60,
        .lfo1_shape = 0, .lfo1_rate = 15, .lfo1_target = LFO_TARGET_ENV, .lfo1_depth = 40,
        .lfo2_shape = 8, .lfo2_rate = 40, .lfo2_target = LFO_TARGET_PITCH, .lfo2_depth = 20,
        .env_shape = 90,
        .resonant_mode = RESONANT_MODE_NOTCH,
        .resonant_res = 80,
        .resonant_center = 50,  // ~4000 Hz
        .engine_map = {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // Preset 7: "Polyrhythm" (original)
    {
        .name = "Polyrhythm",
        .prob_kick = 90, .prob_snare = 70, .prob_metal = 50, .prob_perc = 80,
        .kick_sweep = 50, .kick_decay = 50,
        .snare_noise = 50, .snare_body = 50,
        .metal_inharm = 50, .metal_bright = 50,
        .perc_ratio = 50, .perc_var = 50,
        .lfo1_shape = 4, .lfo1_rate = 35, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 30,
        .lfo2_shape = 7, .lfo2_rate = 55, .lfo2_target = LFO_TARGET_PITCH, .lfo2_depth = 30,
        .env_shape = 25,
        .resonant_mode = RESONANT_MODE_BANDPASS,
        .resonant_res = 50,
        .resonant_center = 30,  // ~2400 Hz
        .engine_map = {ENGINE_KICK, ENGINE_SNARE, ENGINE_METAL, ENGINE_PERC}
    },

    // ===== NEW RESONANT PRESETS =====

    // Preset 8: "ResoKick" - Resonant kick drum
    {
        .name = "ResoKick",
        .prob_kick = 100, .prob_snare = 0, .prob_metal = 0, .prob_perc = 0,
        .kick_sweep = 70, .kick_decay = 60,  // These are ignored for resonant
        .snare_noise = 0, .snare_body = 0,
        .metal_inharm = 0, .metal_bright = 0,
        .perc_ratio = 0, .perc_var = 0,
        .lfo1_shape = 0, .lfo1_rate = 10, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 20,
        .lfo2_shape = 0, .lfo2_rate = 5, .lfo2_target = LFO_TARGET_NONE, .lfo2_depth = 0,
        .env_shape = 25,  // Medium decay
        .resonant_mode = RESONANT_MODE_LOWPASS,
        .resonant_res = 40,
        .resonant_center = 15,  // ~1200 Hz
        .engine_map = {ENGINE_RESONANT, ENGINE_RESONANT, ENGINE_RESONANT, ENGINE_RESONANT}
    },

    // Preset 9: "ResoTom" - Resonant tom
    {
        .name = "ResoTom",
        .prob_kick = 0, .prob_snare = 0, .prob_metal = 0, .prob_perc = 100,
        .kick_sweep = 50, .kick_decay = 50,
        .snare_noise = 0, .snare_body = 0,
        .metal_inharm = 0, .metal_bright = 0,
        .perc_ratio = 50, .perc_var = 50,
        .lfo1_shape = 1, .lfo1_rate = 15, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 30,
        .lfo2_shape = 1, .lfo2_rate = 8, .lfo2_target = LFO_TARGET_INDEX, .lfo2_depth = 20,
        .env_shape = 35,  // Longer decay
        .resonant_mode = RESONANT_MODE_BANDPASS,
        .resonant_res = 60,
        .resonant_center = 25,  // ~2000 Hz
        .engine_map = {ENGINE_RESONANT, ENGINE_RESONANT, ENGINE_RESONANT, ENGINE_RESONANT}
    },

    // Preset 10: "ResoSnare" - Resonant snare
    {
        .name = "ResoSnare",
        .prob_kick = 0, .prob_snare = 100, .prob_metal = 0, .prob_perc = 0,
        .kick_sweep = 0, .kick_decay = 0,
        .snare_noise = 50, .snare_body = 50,
        .metal_inharm = 0, .metal_bright = 0,
        .perc_ratio = 0, .perc_var = 0,
        .lfo1_shape = 2, .lfo1_rate = 20, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 40,
        .lfo2_shape = 2, .lfo2_rate = 10, .lfo2_target = LFO_TARGET_INDEX, .lfo2_depth = 30,
        .env_shape = 20,  // Short decay
        .resonant_mode = RESONANT_MODE_HIGHPASS,
        .resonant_res = 50,
        .resonant_center = 45,  // ~3600 Hz
        .engine_map = {ENGINE_RESONANT, ENGINE_RESONANT, ENGINE_RESONANT, ENGINE_RESONANT}
    },

    // Preset 11: "ResoMetal" - Resonant metal/cymbal
    {
        .name = "ResoMetal",
        .prob_kick = 0, .prob_snare = 0, .prob_metal = 100, .prob_perc = 0,
        .kick_sweep = 0, .kick_decay = 0,
        .snare_noise = 0, .snare_body = 0,
        .metal_inharm = 70, .metal_bright = 80,
        .perc_ratio = 0, .perc_var = 0,
        .lfo1_shape = 8, .lfo1_rate = 30, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 50,
        .lfo2_shape = 6, .lfo2_rate = 40, .lfo2_target = LFO_TARGET_INDEX, .lfo2_depth = 40,
        .env_shape = 10,  // Very short decay
        .resonant_mode = RESONANT_MODE_PEAK,
        .resonant_res = 80,
        .resonant_center = 80,  // ~6400 Hz
        .engine_map = {ENGINE_RESONANT, ENGINE_RESONANT, ENGINE_RESONANT, ENGINE_RESONANT}
    }
};