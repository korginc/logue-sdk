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
        {0, 1000, 500, 500, k_unit_param_type_none, 1, 1, 0, {"MlltRes"}},
        {100, 5000, 250, 2500, k_unit_param_type_none, 0, 0, 0, {"MlltStif"}},
        {-100, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"VlMllRes"}},
        {-100, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"VlMllStf"}},

        // Page 3: Resonator I
        // 0-4 are the partials, value 5 means that next params dedicated to resonator are addressing resonator A and value 6 are for resonator B
        {0, 6, 0, 3, k_unit_param_type_strings, 0, 0, 0, {"Partls"}},
        // [0..8] String , Beam ,  Square , Membrn , Plate , Drumhd , Marmb , OpnTub , ClsTub
        {0, 8, 0, 3, k_unit_param_type_strings, 0, 0, 0, {"Model"}},
        // [0..2000]
        {0, 2000, 250, 250, k_unit_param_type_none, 0, 0, 0, {"Dkay"}},
        // [−10..30]
        {-10, 30, 0, 10, k_unit_param_type_none, 1, 1, 0, {"Mterl"}},

        // Page 4: Resonator II
        {-10, 30, 0, 0, k_unit_param_type_none, 1, 0, 0, {"Tone"}},
        {2, 98, 0, 26, k_unit_param_type_none, 2, 0, 0, {"HitPos"}},
        {0, 20, 0, 10, k_unit_param_type_none, 1, 0, 0, {"Rel"}},
        // [0..1999] — stored value is multiplied by 10 in code (effective range 0–19990).
        // Using 10× coarser steps makes the encoder 10× faster to dial.
        {0, 1999, 300, 300, k_unit_param_type_none, 0, 0, 0, {"Inharm"}},

        // Page 5: Resonator III
        // [1..1999] — stored value is multiplied by 10 in code (effective 10–19990 Hz).
        // type_strings so getParameterStrValue can display the real Hz/kHz value.
        {1, 1999, 500, 1, k_unit_param_type_strings, 0, 0, 0, {"LowCut"}},
        {0, 20, 0, 5, k_unit_param_type_none, 1, 0, 0, {"TubRad"}},
        // Range 0-100, no fraction
        {0, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Gain"}},  // <--- Overdrive
        {0, 100, 50, 0, k_unit_param_type_percent, 0, 0, 0, {"NzMix"}},

        // Page 6: Noise II
        {0, 1000, 300, 0, k_unit_param_type_percent, 1, 1, 0, {"NzRes"}},
        {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"NzFltr"}},
        // Range change deferred: requires dedicated Noise SVF (Phase 13).
        // When implemented: change to 2-2000 (type_strings), multiply by 10 in setParameter.
        {20, 20000, 12000, 20, k_unit_param_type_hertz, 0, 0, 0, {"NzFltFrq"}},
        {707, 4000, 0, 707, k_unit_param_type_none, 0, 0, 0, {"Resnc"}}
    }
};