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
    .unit_id = 0x5U,                                       // Id for this unit, should be unique within the scope of a given dev_id
    .version = 0x00010000U,                                // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "Vibrato",                                     // Name for this unit, will be displayed on device
    .num_presets = 0,                                      // Number of internal presets this unit has
    .num_params = 7,                                       // Number of parameters for this unit, max 8
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        // Page 1
        {0, 1023, 511, 256, k_unit_param_type_none,    0, 0, 0, {"Rate"}},
        {0, 1023, 511, 511, k_unit_param_type_none,    0, 0, 0, {"Depth"}},
        {0,  100,  50,   0, k_unit_param_type_percent, 0, 0, 0, {"Spread"}}, 

        // Page 2
        {0, 1023, 511,   0, k_unit_param_type_none,    0, 0, 0, {"Low Cut"}},
        {0, 1023, 511,   0, k_unit_param_type_none,    0, 0, 0, {"Shape"}},
        {0,    1,   0,   0, k_unit_param_type_onoff,   0, 0, 0, {"BPM Sync"}},
        {0,    1,   0,   0, k_unit_param_type_onoff,   0, 0, 0, {"Deep Fry"}}}};