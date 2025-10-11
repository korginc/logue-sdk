/**
 *  @file header.c
 *  @brief microkorg2 SDK unit header
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"  // Note: Include common definitions for all units
#include "runtime.h"
#include "waves_common.h" // Include common definitions specific to waves oscillator

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),                  // leave as is, size of this header
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,  // target platform and module for this unit
    .api = UNIT_API_VERSION,                               // logue sdk API version against which unit was built
    .dev_id = 0x4B4F5247U,                                 // developer identifier
    .unit_id = 0x050400U,                                  // Id for this unit, should be unique within the scope of a given dev_id
    .version = 0x00020000U,                                // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "waves",                                       // Name for this unit, will be displayed on device
    .num_presets = 0,                                      // Number of internal presets this unit has
    .num_params = 8,                                       // Number of parameters for this unit, max 13
    .params = {
    // Format:
    // min, max, center, default, type, frac. bits, frac. mode, <reserved>, name
    
    // See common/runtime.h for type enum and unit_param_t structure
    
    // Fixed/direct UI parameters
    // Page 1    
    {0, 1023,           0, 0, k_unit_param_type_none,    0, 0, 0, {"SHPE"}},
    {0, 1023,           0, 0, k_unit_param_type_none,    0, 0, 0, {"SUB"}},
    {0, WAVE_A_CNT-1,   0, 0, k_unit_param_type_enum,    0, 0, 0, {"WAVE A"}},

    // Page 2
    {0, WAVE_B_CNT-1,   0, 0, k_unit_param_type_enum,    0, 0, 0, {"WAVE B"}},
    {0, SUB_WAVE_CNT-1, 0, 0, k_unit_param_type_enum,    0, 0, 0, {"SUB WAVE"}},
    {0, 1000,           0, 0, k_unit_param_type_percent, 1, 1, 0, {"RING MIX"}},
    {0, 1000,           0, 0, k_unit_param_type_percent, 1, 1, 0, {"BIT CRSH"}},
    {0, 1000,           0, 250, k_unit_param_type_percent, 1, 1, 0, {"DRIFT"}},

    // page 3
    {0, 0,              0, 0, k_unit_param_type_none,    0, 0, 0, {""}},
    {0, 0,              0, 0, k_unit_param_type_none,    0, 0, 0, {""}}},
    };
