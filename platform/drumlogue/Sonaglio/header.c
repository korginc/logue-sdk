/**
 * @file header.c
 * @brief FM Percussion Synth - One instance per instrument
 */

#include "unit.h"

// String tables
const char* instruments_strings[10] = {
    "Kick",
    "Snare",
    "Tom",
    "Metal",
    "K+S",
    "K+T",
    "K+M",
    "S+T",
    "S+M",
    "T+M"
};

const char* lfo_shape_strings[9] = {
    "Tri+Tri", "Rmp+Rmp", "Chd+Chd",
    "Tri+Rmp", "Tri+Chd", "Rmp+Tri",
    "Rmp+Chd", "Chd+Tri", "Chd+Rmp"
};

const char* lfo_target_strings[11] = {
    "None", "Pitch", "ModIdx", "Env",
    "LFO2Ph", "LFO1Ph", "ResFrq", "Reson",
    "NoizMx", "ResMrph", "MtlGate"
};

const char* euclidean_mode_strings[9] = {
    "Off",    // 0: disabled — all voices same pitch
    "Clstr",  // 1: E(4,4)  = [0, 1, 2, 3]  chromatic cluster
    "Minor",  // 2: E(4,6)  = [0, 1, 3, 4]  minor 3rd pairs
    "Diatn",  // 3: E(4,7)  = [0, 1, 3, 5]  diatonic cluster
    "Whole",  // 4: E(4,8)  = [0, 2, 4, 6]  whole tone
    "Penta",  // 5: E(4,10) = [0, 2, 5, 7]  pentatonic / 5th
    "Dim7",   // 6: E(4,12) = [0, 3, 6, 9]  diminished 7th
    "Aug8",   // 7: E(4,16) = [0, 4, 8, 12] augmented + octave
    "Trit"    // 8: E(4,24) = [0, 6, 12, 18] tritone spread
};

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x46654465U,   // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id = 0x03U,        // 0x02 is FMpercussion; Sonaglio must have a unique unit_id
    .version = 0x00010000U,  // v1.0.0 — required; version=0 causes load rejection on drumlogue
    .name = "Sonaglio",
    .num_presets = 26,
    .num_params = 24,
    .params = {
        // Page 1: Engine probabilities
        // Instrument: single engine or combo pair
        {0,   9, 0,   0, k_unit_param_type_strings, 0, 0, 0, {"Instr"}},
        // Blend: balance between the pair; in single modes, the strength of the delayed shadow
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"Blend"}},
        // Gap: actual separation between primary and secondary hit
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"Gap"}},
        // Scatter: jitter, detune, looseness, and chaos
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"Scatter"}},

        // Page 2: Kick + Snare
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KAtk"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KBody"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"SAtk"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"SBody"}},

        // Page 3: Metal + Perc
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"MAtk"}},
        {0, 100, 0, 70, k_unit_param_type_percent, 0, 0, 0, {"MBody"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"PAtk"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"PBody"}},

        // Page 4: LFO1
        {0, 8,   0, 0, k_unit_param_type_strings,  0, 0, 0, {"L1Shape"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"L1Rate"}},
        {0, 10,  0, 0, k_unit_param_type_strings,  0, 0, 0, {"L1Dest"}},
        {-100, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"L1Depth"}},

        // Page 5: LFO2 + Euclidean Tuning
        // EuclTun: Euclidean per-voice pitch spread. Assigns voice i a semitone
        //   offset = floor(i * n / 4) so the 4 voices are maximally spread across
        //   n chromatic steps. Mode 0 = off (all voices same pitch).
        {0, 8,   0, 0, k_unit_param_type_strings, 0, 0, 0, {"EuclTun"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"L2Rate"}},
        {0, 10,  0, 0, k_unit_param_type_strings, 0, 0, 0, {"L2Dest"}},
        {-100, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"L2Depth"}},

        // Page 6: Envelope + Global shaping
        {0, 255, 0, 40, k_unit_param_type_none,    0, 0, 0, {"EnvShape"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"HitShp"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"BodyTilt"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"Drive"}}
    }
};
