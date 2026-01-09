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
    .target = UNIT_TARGET_PLATFORM | k_unit_module_revfx,  // target platform and module for this unit
    .api = UNIT_API_VERSION,                               // logue sdk API version against which unit was built
    .dev_id = 0x4B4F5247U,                                 // developer identifier
    .unit_id = 0x3U,                                       // Id for this unit, should be unique within the scope of a given dev_id
    .version = 0x00010000U,                                // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "breveR",                                      // Name for this unit, will be displayed on device
    .num_presets = 0,                                      // Number of internal presets this unit has
    .num_params = 8,                                       // Number of parameters for this unit, max 24
    .params = {
        // Format: min, max, center, default, type, fractional, frac. type, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        // Page 1
        {0,    100,  0,  50, k_unit_param_type_none,    0, 1, 0, {"Time"}},
        {0,    100,  0,  50, k_unit_param_type_none,    0, 1, 0, {"Depth"}},
        {0,    100,  0,  20, k_unit_param_type_none,    0, 1, 0, {"HiDamp"}},

        // Page 2
        {  1, 6000,  0, 300, k_unit_param_type_msec,    1, 1, 0, {"PreDelay"}}, 
        {   0, 100,  0, 100, k_unit_param_type_none,    0, 0, 0, {"Size"}},
        {   0, 100,  0, 100, k_unit_param_type_none,    0, 0, 0, {"Diffuse"}},
        {   0,   1,  0,   1, k_unit_param_type_strings, 0, 0, 0, {"Reverse"}},
        {   0, 100,  0,  50, k_unit_param_type_none,    0, 0, 0, {"Mix"}}}};