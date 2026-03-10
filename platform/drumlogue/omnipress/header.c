/**
 * @file header.c
 * @brief drumlogue SDK unit header for OmniPress Master Compressor
 */

#include "unit.h"

// Struct layout: min, max, center, init, type, frac, frac_mode, reserved, name

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target      = UNIT_TARGET_PLATFORM | k_unit_module_masterfx,
    .api         = UNIT_API_VERSION,
    .dev_id      = 0x46654465U,   // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id     = 0x03U,
    .version     = 0x00020000U,   // v2.0.0
    .name        = "OmniPress",
    .num_presets = 0,
    .num_params  = 13,

    .params = {
        // Page 1: Core Dynamics
        // ID 0: THRESH  -60.0..0.0 dB  (x0.1, stored -600..0)
        { -600, 0, -600, -200, k_unit_param_type_db, 1, 1, 0, {"THRESH"} },
        // ID 1: RATIO   1.0:1..20.0:1  (x0.1, stored 10..200)
        { 10, 200, 10, 40, k_unit_param_type_none, 1, 1, 0, {"RATIO"} },
        // ID 2: ATTACK  1..1000 ms  (x0.1 ms, stored 1..1000)
        { 1, 1000, 1, 150, k_unit_param_type_msec, 1, 1, 0, {"ATTACK"} },
        // ID 3: RELEASE 10..2000 ms
        { 10, 2000, 10, 200, k_unit_param_type_msec, 0, 0, 0, {"RELEASE"} },

        // Page 2: Character & Output
        // ID 4: MAKEUP  0.0..24.0 dB  (x0.1, stored 0..240)
        { 0, 240, 0, 0, k_unit_param_type_db, 1, 1, 0, {"MAKEUP"} },
        // ID 5: DRIVE   0-100%
        { 0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"DRIVE"} },
        // ID 6: MIX  -100 (DRY) .. +100 (WET)
        { -100, 100, 0, 100, k_unit_param_type_drywet, 0, 0, 0, {"MIX"} },
        // ID 7: SC HPF  20..500 Hz
        { 20, 500, 20, 20, k_unit_param_type_hertz, 0, 0, 0, {"SC HPF"} },

        // Page 3: Mode Selection
        // ID 8: COMP MODE  0=Standard, 1=Distressor, 2=Multiband
        { 0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"COMP MODE"} },
        // ID 9: BAND SEL  0=Low, 1=Mid, 2=High, 3=All
        { 0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"BAND SEL"} },
        // ID 10: L THRESH  -60.0..0.0 dB  (per-band threshold)
        { -600, 0, -600, -200, k_unit_param_type_db, 1, 1, 0, {"L THRESH"} },
        // ID 11: L RATIO   1.0..20.0:1
        { 10, 200, 10, 40, k_unit_param_type_none, 1, 1, 0, {"L RATIO"} },

        // Page 4: Multiband / Distressor Parameters
        // ID 12: DSTR MODE  0=None, 1=2nd harm, 2=3rd harm, 3=Both
        { 0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"DSTR MODE"} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },

        // Pages 5-6: blank padding to fill 24 slots
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
    }
};
