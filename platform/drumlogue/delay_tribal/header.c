/**
 * @file header.c
 * @brief drumlogue SDK unit header for Enhanced Percussion Spatializer
 */

#include "unit.h"

// Struct layout: min, max, center, init, type, frac, frac_mode, reserved, name

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target      = UNIT_TARGET_PLATFORM | k_unit_module_delfx,
    .api         = UNIT_API_VERSION,
    .dev_id      = 0x46654465U,   // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id     = 0x01U,
    .version     = 0x00030000U,   // v3.0.0 - new design
    .name        = "Tribale",
    .num_presets = 0,
    .num_params  = 9,

    .params = {
        // Page 1
        // ID 0: Clones  0=2, 1=4, 2=6, 3=8, 4=10
        { 0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Clones"} },
        // ID 1: Mode  0=Tribal, 1=Military, 2=Angel
        { 0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Mode"} },
        // ID 2: Depth  0-100%
        { 0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"Depth"} },
        // ID 3: Rate  x0.1 precision (0.0..10.0 Hz)
        { 0, 100, 0, 30, k_unit_param_type_none, 1, 0, 0, {"Rate"} },

        // Page 2
        // ID 4: Spread  0-100%
        { 0, 100, 0, 80, k_unit_param_type_percent, 0, 0, 0, {"Spread"} },
        // ID 5: Mix  0-100%
        { 0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"Mix"} },
        // ID 6: Wobble (Pitch Wobble Depth)  0-100%
        { 0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"Wobble"} },
        // ID 7: Scatter (ensemble looseness)  0-100%
        { 0, 100, 0, 20, k_unit_param_type_percent, 0, 0, 0, {"Scatter"} },

        // Pages 3-6: blank padding to fill 24 slots
        // ID 8: SoftAtk (Attack Softening)  0-100%
        { 0, 100, 0, 20, k_unit_param_type_percent, 0, 0, 0, {"SoftAtk"} },
        // ID 9: Gap (distance between hits)  0-100%
        { 0, 100, 0, 20, k_unit_param_type_percent, 0, 0, 0, {"Gap"} },
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
