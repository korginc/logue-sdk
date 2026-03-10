/**
 * @file header.c
 * @brief drumlogue SDK unit header for NeonLabirinto reverb
 */

#include "unit.h"

// Struct layout: min, max, center, init, type, frac, frac_mode, reserved, name

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target      = UNIT_TARGET_PLATFORM | k_unit_module_revfx,
    .api         = UNIT_API_VERSION,
    .dev_id      = 0x46654465U,   // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id     = 0x00010000U,
    .version     = 0x00010000U,   // v1.0.0
    .name        = "NeonLabirinto",
    .num_presets = 0,
    .num_params  = 8,
    .params = {
        // Page 1: Main reverb controls
        // ID 0: MIX  wet/dry blend  0.0%-100.0% (x0.1 precision, stored 0..1000)
        { 0, 1000, 500, 300, k_unit_param_type_percent, 1, 0, 0, {"MIX"} },
        // ID 1: TIME mid RT60 x0.1 s  (1=0.1s .. 100=10.0s)
        { 1, 100, 20, 20, k_unit_param_type_none, 0, 0, 0, {"TIME"} },
        // ID 2: LOW  low-freq RT60 x0.1 s
        { 1, 100, 20, 20, k_unit_param_type_none, 0, 0, 0, {"LOW"} },
        // ID 3: HIGH high-freq RT60 x0.1 s
        { 1, 100, 20, 10, k_unit_param_type_none, 0, 0, 0, {"HIGH"} },

        // Page 2: Advanced controls
        // ID 4: DAMP damping crossover frequency  200..10000 Hz
        { 200, 10000, 2500, 2500, k_unit_param_type_none, 0, 0, 0, {"DAMP"} },
        // ID 5: WIDE stereo width  0%-200%
        { 0, 200, 100, 100, k_unit_param_type_percent, 0, 0, 0, {"WIDE"} },
        // ID 6: COMP diffusion/complexity  0.0%-100.0% (x0.1, stored 0..1000)
        { 0, 1000, 500, 1000, k_unit_param_type_percent, 1, 0, 0, {"COMP"} },
        // ID 7: PILL pillar count index  0=4, 1=8, 2=16, 3=32
        { 0, 3, 3, 3, k_unit_param_type_none, 0, 0, 0, {"PILL"} },

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
    }
};
