/**
 * @file header.c
 * @brief drumlogue SDK unit header for OmniPress Master Compressor
 *
 * Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"  // Note: Include common definitions for all units

// ---- Unit header definition  --------------------------------------------------------------------

// String tables for parameter display
static const char* comp_mode_strings[3] = {
    "Standard",
    "Distressor",
    "Multiband"
};

static const char* band_select_strings[4] = {
    "Low",
    "Mid",
    "High",
    "All"
};

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_masterfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x46654465U,       // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id = 0x03U,
    .version = 0x00020000U,      // v2.0.0 with multiband
    .name = "OmniPress",
    .num_presets = 0,
    .num_params = 12,  // Now using 12 parameters

    .params = {
        // --- Page 1: Core Dynamics ---
        { -600, 0, -600, -200, k_unit_param_type_db, 1, 1, 0, {"THRESH"} },
        { 10, 200, 10, 40, k_unit_param_type_none, 1, 1, 0, {"RATIO"} },
        { 1, 1000, 1, 150, k_unit_param_type_msec, 1, 1, 0, {"ATTACK"} },
        { 10, 2000, 10, 200, k_unit_param_type_msec, 0, 0, 0, {"RELEASE"} },

        // --- Page 2: Character & Output ---
        { 0, 240, 0, 0, k_unit_param_type_db, 1, 1, 0, {"MAKEUP"} },
        { 0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"DRIVE"} },
        { -100, 100, 0, 100, k_unit_param_type_drywet, 0, 0, 0, {"MIX"} },
        { 20, 500, 20, 20, k_unit_param_type_hertz, 0, 0, 0, {"SC HPF"} },

        // --- Page 3: Mode Selection ---
        { 0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"COMP MODE"} },  // Param 8
        { 0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"BAND SEL"} },   // Param 9
        { -600, 0, -600, -200, k_unit_param_type_db, 1, 1, 0, {"L THRESH"} },  // Param 10
        { 10, 200, 10, 40, k_unit_param_type_none, 1, 1, 0, {"L RATIO"} },     // Param 11

        // --- Page 4: Multiband Parameters ---
        { 0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"DSTR MODE"} }  // None, 2nd harmonics, 3rd, both
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        // ... (continues to 24)
    }
};

const char* unit_get_param_str_value(uint8_t index, int32_t value) {
    static char buf[16];

    switch (index) {
        case 8: // COMP MODE
            if (value >= 0 && value <= 2)
                return comp_mode_strings[value];
            break;
        case 9: // BAND SEL
            if (value >= 0 && value <= 3)
                return band_select_strings[value];
            break;
        case 10: // L THRESH - could format with "dB"
        case 11: // L RATIO - could format with ":1"
            snprintf(buf, sizeof(buf), "%d", value);
            return buf;
    }
    return nullptr;
}