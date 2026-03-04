/**
 *  @file fm_presets.h
 *  @brief Factory presets for FM Percussion Synth
 *
 *  8 presets demonstrating LFO capabilities
 */

#pragma once

#include <stdint.h>

// LFO target values (from lfo_enhanced.h)
#define LFO_TARGET_NONE     0
#define LFO_TARGET_PITCH    1
#define LFO_TARGET_INDEX    2
#define LFO_TARGET_ENV      3
#define LFO_TARGET_LFO2_PHASE 4
#define LFO_TARGET_LFO1_PHASE 5

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
} fm_preset_t;

// Factory presets
static const fm_preset_t FM_PRESETS[8] = {
    {   // Preset 0: "Deep Tribal"
        .name = "Deep Tribal",
        .prob_kick = 100, .prob_snare = 80, .prob_metal = 60, .prob_perc = 70,
        .kick_sweep = 80, .kick_decay = 60,
        .snare_noise = 30, .snare_body = 70,
        .metal_inharm = 40, .metal_bright = 50,
        .perc_ratio = 50, .perc_var = 40,
        .lfo1_shape = 0, .lfo1_rate = 20, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 30,
        .lfo2_shape = 1, .lfo2_rate = 5, .lfo2_target = LFO_TARGET_INDEX, .lfo2_depth = 20,
        .env_shape = 20
    },
    {   // Preset 1: "Metal Storm"
        .name = "Metal Storm",
        .prob_kick = 60, .prob_snare = 80, .prob_metal = 100, .prob_perc = 50,
        .kick_sweep = 40, .kick_decay = 30,
        .snare_noise = 60, .snare_body = 40,
        .metal_inharm = 80, .metal_bright = 90,
        .perc_ratio = 30, .perc_var = 60,
        .lfo1_shape = 6, .lfo1_rate = 60, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 20,
        .lfo2_shape = 4, .lfo2_rate = 80, .lfo2_target = LFO_TARGET_LFO1_PHASE, .lfo2_depth = 50,
        .env_shape = 5
    },
    {   // Preset 2: "Chordal Perc"
        .name = "Chordal Perc",
        .prob_kick = 70, .prob_snare = 70, .prob_metal = 70, .prob_perc = 70,
        .kick_sweep = 50, .kick_decay = 50,
        .snare_noise = 40, .snare_body = 50,
        .metal_inharm = 30, .metal_bright = 40,
        .perc_ratio = 60, .perc_var = 50,
        .lfo1_shape = 2, .lfo1_rate = 30, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 40,
        .lfo2_shape = 2, .lfo2_rate = 15, .lfo2_target = LFO_TARGET_PITCH, .lfo2_depth = -30,
        .env_shape = 40
    },
    {   // Preset 3: "Phase Dance"
        .name = "Phase Dance",
        .prob_kick = 80, .prob_snare = 60, .prob_metal = 80, .prob_perc = 60,
        .kick_sweep = 30, .kick_decay = 40,
        .snare_noise = 50, .snare_body = 60,
        .metal_inharm = 50, .metal_bright = 60,
        .perc_ratio = 40, .perc_var = 30,
        .lfo1_shape = 3, .lfo1_rate = 45, .lfo1_target = LFO_TARGET_LFO2_PHASE, .lfo1_depth = 70,
        .lfo2_shape = 5, .lfo2_rate = 30, .lfo2_target = LFO_TARGET_LFO1_PHASE, .lfo2_depth = 70,
        .env_shape = 30
    },
    {   // Preset 4: "Bipolar Bass"
        .name = "Bipolar Bass",
        .prob_kick = 100, .prob_snare = 40, .prob_metal = 30, .prob_perc = 50,
        .kick_sweep = 90, .kick_decay = 80,
        .snare_noise = 20, .snare_body = 30,
        .metal_inharm = 20, .metal_bright = 30,
        .perc_ratio = 30, .perc_var = 20,
        .lfo1_shape = 1, .lfo1_rate = 10, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = -50,
        .lfo2_shape = 0, .lfo2_rate = 25, .lfo2_target = LFO_TARGET_NONE, .lfo2_depth = 0,
        .env_shape = 10
    },
    {   // Preset 5: "Snare Roll"
        .name = "Snare Roll",
        .prob_kick = 30, .prob_snare = 100, .prob_metal = 40, .prob_perc = 30,
        .kick_sweep = 20, .kick_decay = 30,
        .snare_noise = 80, .snare_body = 60,
        .metal_inharm = 30, .metal_bright = 40,
        .perc_ratio = 40, .perc_var = 30,
        .lfo1_shape = 1, .lfo1_rate = 90, .lfo1_target = LFO_TARGET_INDEX, .lfo1_depth = 60,
        .lfo2_shape = 1, .lfo2_rate = 45, .lfo2_target = LFO_TARGET_PITCH, .lfo2_depth = 20,
        .env_shape = 15
    },
    {   // Preset 6: "Ambient Metals"
        .name = "Ambient Mtl",
        .prob_kick = 40, .prob_snare = 50, .prob_metal = 100, .prob_perc = 60,
        .kick_sweep = 30, .kick_decay = 70,
        .snare_noise = 40, .snare_body = 50,
        .metal_inharm = 70, .metal_bright = 80,
        .perc_ratio = 50, .perc_var = 60,
        .lfo1_shape = 0, .lfo1_rate = 15, .lfo1_target = LFO_TARGET_ENV, .lfo1_depth = 40,
        .lfo2_shape = 8, .lfo2_rate = 40, .lfo2_target = LFO_TARGET_PITCH, .lfo2_depth = 20,
        .env_shape = 90
    },
    {   // Preset 7: "Polyrhythm"
        .name = "Polyrhythm",
        .prob_kick = 90, .prob_snare = 70, .prob_metal = 50, .prob_perc = 80,
        .kick_sweep = 50, .kick_decay = 50,
        .snare_noise = 50, .snare_body = 50,
        .metal_inharm = 50, .metal_bright = 50,
        .perc_ratio = 50, .perc_var = 50,
        .lfo1_shape = 4, .lfo1_rate = 35, .lfo1_target = LFO_TARGET_PITCH, .lfo1_depth = 30,
        .lfo2_shape = 7, .lfo2_rate = 55, .lfo2_target = LFO_TARGET_PITCH, .lfo2_depth = 30,
        .env_shape = 25
    }
};