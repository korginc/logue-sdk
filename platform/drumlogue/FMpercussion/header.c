/**
 * @file header.c
 * @brief FM Percussion Synth - One instance per instrument
 */

#include "unit.h"
#include "constants.h"

// String tables
static const char* lfo_shape_strings[9] = {
    "Tri+Tri", "Ramp+Ramp", "Chord+Chord",
    "Tri+Ramp", "Tri+Chord", "Ramp+Tri",
    "Ramp+Chord", "Chord+Tri", "Chord+Ramp"
};

static const char* lfo_target_strings[8] = {
    "None", "Pitch", "ModIdx", "Env",
    "LFO2Ph", "LFO1Ph", "ResFreq", "Resonance"
};

static const char* resonant_mode_strings[5] = {
    "LowPass", "BandPass", "HighPass", "Notch", "Peak"
};

// Voice allocation - 12 valid combinations (no duplicates)
static const char* voice_alloc_strings[12] = {
    "K-S-M-P",  // 0: Classic (no resonant)
    "K-S-M-R",  // 1: Resonant on Perc
    "K-S-R-P",  // 2: Resonant on Metal
    "K-R-M-P",  // 3: Resonant on Snare
    "R-S-M-P",  // 4: Resonant on Kick
    "K-S-R-M",  // 5: Resonant on Metal, Perc→Metal
    "K-R-S-P",  // 6: Resonant on Snare, Metal→Snare
    "R-K-M-P",  // 7: Resonant on Kick, Snare→Kick
    "R-S-K-P",  // 8: Resonant on Kick, Metal→Kick
    "M-R-K-P",  // 9: Resonant on Snare, Kick→Snare
    "P-R-K-M",  // 10: Resonant on Snare, full rotation
    "M-P-R-K"   // 11: Full rotation - all moved
};

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x46654465U,   // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id = 0x02U,
    .version = 0x00020000U,
    .name = "FMPerc",
    .num_presets = 12,
    .num_params = 24,

    .params = {
        // Page 1: Voice Probabilities
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"V1Prob"}},
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"V2Prob"}},
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"V3Prob"}},
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"V4Prob"}},

        // Page 2: Kick + Snare
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KSweep"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KDecay"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"SNoise"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"SBody"}},

        // Page 3: Metal + Perc
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"MInharm"}},
        {0, 100, 0, 70, k_unit_param_type_percent, 0, 0, 0, {"MBrght"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"PRatio"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"PVar"}},

        // Page 4: LFO1
        {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L1Shape"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"L1Rate"}},
        {0, 7, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L1Dest"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"L1Depth"}},

        // Page 5: LFO2
        {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L2Shape"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"L2Rate"}},
        {0, 7, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L2Dest"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"L2Depth"}},

        // Page 6: Envelope + Voice + Resonant
        {0, 127, 0, 40, k_unit_param_type_none, 0, 0, 0, {"EnvShape"}},
        {0, 11, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"VoiceAlloc"}},
        {0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"ResMode"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"ResMorph"}}
    }
};