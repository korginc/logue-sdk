/**
 * @file header.c
 * @brief drumlogue SDK unit header for OmniPress Master Compressor
 *
 * Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"  // Note: Include common definitions for all units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),                     // leave as is, size of this header
    .target = UNIT_TARGET_PLATFORM | k_unit_module_masterfx,  // target platform and module for this unit
    .api = UNIT_API_VERSION,                                  // logue sdk API version against which unit was built
    .dev_id = 0x0U,                                           // developer id (replace with your unique ID if distributing)
    .unit_id = 0x01U,                                         // Id for this unit, should be unique within the scope of a given dev_id
    .version = 0x00010000U,                                   // This unit's version: major.minor.patch (v1.0.0)
    .name = "OmniPress",                                      // Name for this unit, will be displayed on device (max 13 chars)
    .num_presets = 0,                                         // Number of internal presets this unit has
    .num_params = 24,                                         // Number of parameters for this unit, max 24
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name
        // frac_mode: 0: fixed point, 1: decimal

        // --- Page 1: Core Dynamics ---
        
        // PARAM 1: Threshold (-60.0 dB to 0.0 dB)
        // Uses 1 decimal place. Init: -20.0 dB.
        {-600, 0, -600, -200, k_unit_param_type_db, 1, 1, 0, {"THRESH"}},
        
        // PARAM 2: Ratio (1.0 to 20.0)
        // Uses 1 decimal place. Init: 4.0. 
        {10, 200, 10, 40, k_unit_param_type_none, 1, 1, 0, {"RATIO"}},
        
        // PARAM 3: Attack (0.1 ms to 100.0 ms)
        // Uses 1 decimal place. Init: 15.0 ms.
        {1, 1000, 1, 150, k_unit_param_type_msec, 1, 1, 0, {"ATTACK"}},
        
        // PARAM 4: Release (10 ms to 2000 ms)
        // Integer ms. Init: 200 ms.
        {10, 2000, 10, 200, k_unit_param_type_msec, 0, 0, 0, {"RELEASE"}},

        // --- Page 2: Character & Output ---
        
        // PARAM 5: Make-Up Gain (0.0 dB to 24.0 dB)
        // Uses 1 decimal place. Init: 0.0 dB.
        {0, 240, 0, 0, k_unit_param_type_db, 1, 1, 0, {"MAKEUP"}},
        
        // PARAM 6: Drive / Wavefolder amount (0% to 100%)
        // Integer %. Init: 0%.
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"DRIVE"}},
        
        // PARAM 7: Mix Dry/Wet (-100 to +100)
        // -100 = D (Dry), 0 = BAL, +100 = W (Wet). Init: 100 (100% Wet).
        {-100, 100, 0, 100, k_unit_param_type_drywet, 0, 0, 0, {"MIX"}},
        
        // PARAM 8: Sidechain High-Pass Filter (20 Hz to 500 Hz)
        // Integer Hz. Init: 20 Hz.
        {20, 500, 20, 20, k_unit_param_type_hertz, 0, 0, 0, {"SC HPF"}},

        // --- Page 3 to 6: Unused (Blanked out to clean up UI) ---
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
};