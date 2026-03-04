/**
 *  @file header.c
 *  @brief drumlogue SDK unit header
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 *  @brief 4-Voice FM Percussion Synthesizer for drumlogue
 *
 *  4 independent FM engines with combinatorial LFO control
 *  Version 1.0
 */

#include "unit.h"
#include "constants.h"

// String tables for parameter display
static const char* lfo_shape_strings[9] = {
    "Tri+Tri", "Ramp+Ramp", "Chord+Chord",
    "Tri+Ramp", "Tri+Chord", "Ramp+Tri",
    "Ramp+Chord", "Chord+Tri", "Chord+Ramp"
};

static const char* voice_mask_strings[15] = {
    "Kick", "Snare", "Metal", "Perc",
    "K+S", "K+M", "K+P", "S+M",
    "S+P", "M+P", "K+S+M", "K+S+P",
    "K+M+P", "S+M+P", "All"
};

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_delfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x7D0U,           // Same developer ID as spatializer
    .unit_id = 0x02U,            // Second unit
    .version = 0x00010000U,      // v1.0.0
    .name = "FMPerc",
    .num_presets = 8,
    .num_params = 24,

    .params = {
        // ---------- Page 1: Trigger Probabilities ----------
        // Param 1-1: Kick Probability
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"ProbK"}},
        // Param 1-2: Snare Probability
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"ProbS"}},
        // Param 1-3: Metal Probability
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"ProbM"}},
        // Param 1-4: Perc Probability
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"ProbP"}},

        // ---------- Page 2: Kick + Snare Parameters ----------
        // Param 2-1: Kick Sweep Depth (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KSweep"}},
        // Param 2-2: Kick Decay Shape (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KDecay"}},
        // Param 2-3: Snare Noise Mix (0-100%)
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"SNoise"}},
        // Param 2-4: Snare Body Resonance (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"SBody"}},

        // ---------- Page 3: Metal + Perc Parameters ----------
        // Param 3-1: Metal Inharmonicity (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"MInharm"}},
        // Param 3-2: Metal Brightness (0-100%)
        {0, 100, 0, 70, k_unit_param_type_percent, 0, 0, 0, {"MBrght"}},
        // Param 3-3: Perc Ratio Center (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"PRatio"}},
        // Param 3-4: Perc Variation (0-100%)
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"PVar"}},

        // ---------- Page 4: LFO1 Configuration ----------
        // Param 4-1: LFO1 Shape (0-8, string display)
        {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L1Shape"}},
        // Param 4-2: LFO1 Rate (0-100%)
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"L1Rate"}},
        // Param 4-3: LFO1 Voice Mask (0-14, string display)
        {0, 14, 0, 14, k_unit_param_type_strings, 0, 0, 0, {"L1Dest"}},
        // Param 4-4: LFO1 Modulation Depth (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"L1Depth"}},

        // ---------- Page 5: LFO2 Configuration ----------
        // Param 5-1: LFO2 Shape (0-8, string display)
        {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L2Shape"}},
        // Param 5-2: LFO2 Rate (0-100%)
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"L2Rate"}},
        // Param 5-3: LFO2 Voice Mask (0-14, string display)
        {0, 14, 0, 14, k_unit_param_type_strings, 0, 0, 0, {"L2Dest"}},
        // Param 5-4: LFO2 Modulation Depth (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"L2Depth"}},

        // ---------- Page 6: Global Envelope + Future ----------
        // Param 6-1: Envelope Shape (0-127, selects from ROM)
        {0, 127, 0, 40, k_unit_param_type_none, 0, 0, 0, {"EnvShape"}},
        // Param 6-2: (unused - future expansion)
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        // Param 6-3: (unused - future expansion)
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        // Param 6-4: (unused - future expansion)
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
};

// String display callbacks
const char* unit_get_param_str_value(uint8_t index, int32_t value) {
    switch (index) {
        // LFO1 Shape (param 12, zero-based)
        case 12: case 16:  // LFO1 Shape and LFO2 Shape
            if (value >= 0 && value <= 8) return lfo_shape_strings[value];
            break;
        // LFO1 Destination (param 14)
        case 14: case 18:  // LFO1 Dest and LFO2 Dest
            if (value >= 0 && value <= 14) return voice_mask_strings[value];
            break;
    }
    return nullptr;
}