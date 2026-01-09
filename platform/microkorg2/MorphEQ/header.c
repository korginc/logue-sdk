/**
 *  @file header.c
 *  @brief microkorg2 SDK unit header
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"  // Note: Include common definitions for all units
#include "runtime.h"

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),                  // leave as is, size of this header
    .target = UNIT_TARGET_PLATFORM | k_unit_module_modfx,  // target platform and module for this unit
    .api = UNIT_API_VERSION,                               // logue sdk API version against which unit was built
    .dev_id = 0x4B4F5247U,                                 // developer identifier
    .unit_id = 0x1U,                                       // Id for this unit, should be unique within the scope of a given dev_id
    .version = 0x00010000U,                                // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "MorphEQ",                                     // Name for this unit, will be displayed on device
    .num_presets = 0,                                      // Number of internal presets this unit has
    .num_params = 8,                                       // Number of parameters for this unit, max 8
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        {-200,   200,   0,  100, k_unit_param_type_none,    0, 0, 0, {"Depth"}},
        {-200,   200,   0,  100, k_unit_param_type_none,    0, 0, 0, {"Cutoff"}},
        {-100,   100,   0,   50, k_unit_param_type_none,    0, 0, 0, {"Spread"}},

        {-120,   120,   0,   30, k_unit_param_type_db,      1, 1, 0, {"Lo Gain"}},
        {   0,  1023, 512,  250, k_unit_param_type_strings, 0, 0, 0, {"Mid Fc"}},
        {   0,   100,   0,   75, k_unit_param_type_none,    0, 0, 0, {"Mid Q"}},
        {-120,   120,   0,   60, k_unit_param_type_db,      1, 1, 0, {"Mid Gain"}}, 
        {-120,   120,   0,  -15, k_unit_param_type_db,      1, 1, 0, {"Hi Gain"}}, 
}};