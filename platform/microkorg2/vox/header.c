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
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,    // target platform and module for this unit
    .api = UNIT_API_VERSION,                               // logue sdk API version against which unit was built
    .dev_id = 0x4B4F5247U,                                 // developer identifier
    .unit_id = 0x4U,                                       // Id for this unit, should be unique within the scope of a given dev_id
    .version = 0x00010000U,                                // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "vox",                                         // Name for this unit, will be displayed on device
    .num_params = 13,                                      // Number of parameters for this unit, max 13
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        // Page 1
        {0, 400, 200, 0, k_unit_param_type_strings, 2, 1, 0, {"Syllable"}},
        {-50, 50, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"Formant"}},
        {10, 100, 50, 50, k_unit_param_type_none, 1, 1, 0, {"Reso"}},

        // Page 2
        {0,  100, 50, 100, k_unit_param_type_none, 0, 0, 0, {"Shape"}},
        {-24, 24, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Semi"}},
        {-50, 50, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Fine"}},
        {0,  100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"NoiseMix"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        // Page 3
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}}};
