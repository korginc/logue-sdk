/**
 * @file header.c
 * @brief drumlogue SDK unit header
 *
 * Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x46654465U,                                 // 'FeDe'
    .unit_id = 0x5265736fU,                                // 'Reso'
    .version = 0x00010000U,
    .name = "RipplerX",
    .num_presets = 28,
    .num_params = 24,
    .params = {
        // Format: min, max, center, default, type, frac_digits, frac_type, <reserved>, name

        // Page 1: Program and sample selection
        {0, 27, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Program"}},
        {24, 126, 1, 60, k_unit_param_type_midi_note, 0, 0, 0, {"Note"}},
        {0, 6, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Bank"}},
        {1, 128, 1, 1, k_unit_param_type_none, 0, 0, 0, {"Sample"}},

        // Page 2: Mallet
        {0, 1000, 500, 8, k_unit_param_type_none, 1, 1, 0, {"MlltRes"}},
        {100, 5000, 250, 250, k_unit_param_type_none, 0, 0, 0, {"MlltStif"}},
        {-100, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"VlMllRes"}},
        {-100, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"VlMllStf"}},

        // Page 3: Resonator A-I
        {0, 17, 0, 3, k_unit_param_type_strings, 0, 0, 0, {"Model"}},
        {0, 4, 0, 3, k_unit_param_type_strings, 0, 0, 0, {"Partls"}},
        {0, 2000, 250, 250, k_unit_param_type_none, 0, 0, 0, {"Dkay"}}, // Removed fractional
        {-10, 30, 0, 0, k_unit_param_type_none, 1, 1, 0, {"Mterl"}},

        // Page 4: Resonator A-II
        {-10, 30, 0, 0, k_unit_param_type_none, 1, 0, 0, {"Tone"}},
        {2, 98, 0, 26, k_unit_param_type_none, 2, 0, 0, {"HitPos"}},
        {0, 20, 0, 10, k_unit_param_type_none, 1, 0, 0, {"Rel"}},
        {1, 19999, 3000, 1, k_unit_param_type_none, 4, 1, 0, {"Inharm"}},

        // Page 5: Resonator A-III
        {10, 19990, 5005, 10, k_unit_param_type_hertz, 0, 0, 0, {"LowCut"}},
        {0, 20, 0, 5, k_unit_param_type_none, 1, 0, 0, {"TubRad"}},
        // Range 0-100, no fraction
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Gain"}},  // <--- Overdrive
        {0, 100, 50, 0, k_unit_param_type_percent, 0, 0, 0, {"NzMix"}},

        // Page 6: Noise II
        {0, 1000, 300, 0, k_unit_param_type_percent, 1, 1, 0, {"NzRes"}},
        {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"NzFltr"}},
        {20, 20000, 12000, 20, k_unit_param_type_hertz, 0, 0, 0, {"NzFltFrq"}},
        {707, 4000, 0, 707, k_unit_param_type_none, 0, 0, 0, {"Resnc"}}
    }
};