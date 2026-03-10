/**
 * @file header.c
 * @brief drumlogue SDK unit header for LuceAlNeon (Light FDN reverb)
 */

#include "unit.h"

// Struct layout: min, max, center, init, type, frac, frac_mode, reserved, name

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target      = UNIT_TARGET_PLATFORM | k_unit_module_revfx,
    .api         = UNIT_API_VERSION,
    .dev_id      = 0x46654465U,   // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id     = 0x00020000U,
    .version     = 0x00010000U,   // v1.0.0
    .name        = "LuceAlNeon",
    .num_presets = 0,
    .num_params  = 6,
    .params = {
        // Page 1
        // ID 0: DARK  decay/warmth  0%-100%
        { 0, 100, 50, 20, k_unit_param_type_percent, 0, 0, 0, {"DARK"} },
        // ID 1: BRIG  brightness (high-freq level)  0%-100%
        { 0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"BRIG"} },
        // ID 2: GLOW  wet/dry mix  0%-100%
        { 0, 100, 50, 30, k_unit_param_type_percent, 0, 0, 0, {"GLOW"} },
        // ID 3: COLR  tone color (LPF amount)  0%-100%
        { 0, 100, 50, 10, k_unit_param_type_percent, 0, 0, 0, {"COLR"} },

        // Page 2
        // ID 4: SPRK  sparkle / modulation depth  0%-100%
        { 0, 100, 50, 5, k_unit_param_type_percent, 0, 0, 0, {"SPRK"} },
        // ID 5: SIZE  room size (delay scale)  0%-100%
        { 0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"SIZE"} },

        // Pages 3-6: blank
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
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
