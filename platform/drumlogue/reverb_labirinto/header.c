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
    .num_params  = 11,
    .params = {
        // Page 1: Main reverb controls
        // ID 0: Preset name
        { 0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Preset"} },
        // ID 1: TIME mid RT60 x0.1 s  (1=0.1s .. 100=10.0s)
        { 1, 100, 20, 50, k_unit_param_type_none, 0, 0, 0, {"TIME"} },
        // ID 2: LOW  low-freq RT60 x0.1 s
        { 1, 100, 20, 50, k_unit_param_type_none, 0, 0, 0, {"LOW"} },
        // ID 3: HIGH high-freq RT60 x0.1 s
        { 1, 100, 50, 70, k_unit_param_type_none, 0, 0, 0, {"HIGH"} },

        // Page 2: Advanced controls
        // ID 4: DAMP damping crossover  20..1000 (unit.cc multiplies by 10 → 200..10000 Hz)
        { 20, 1000, 250, 250, k_unit_param_type_none, 0, 0, 0, {"DAMP"} },
        // ID 5: WIDE stereo width  0%-200%
        { 0, 200, 100, 100, k_unit_param_type_percent, 0, 0, 0, {"WIDE"} },
        // ID 6: DFSN diffusion/complexity  0%-100%
        { 0, 100, 50, 100, k_unit_param_type_percent, 0, 0, 0, {"DFSN"} },
        // ID 7: PILL pillar count index  0=sparse(2ch), 1=ping-pong(4ch), 2=stone(6ch), 3=full(8ch), 4=shimmer(8ch+)
        { 0, 4, 3, 3, k_unit_param_type_none, 0, 0, 0, {"PILL"} },

        // Page 3:
        // ID 8: SHMR shimmer frequency for microtonal low pitch shimmer
        { 0, 100, 50, 35, k_unit_param_type_strings, 0, 0, 0, {"SHMR"} },
        // ID 9: PDLY  pre-delay time 0..200 ms
        { 0, 200, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PDLY"} },
        // ID 10: VIBR  LFO speed for random modulation (0.1-3.0 Hz, stored as 1-30)
        { 1, 30, 10, 10, k_unit_param_type_none, 0, 0, 0, {"VIBR"} },
        // Pages 3-6: blank padding
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
