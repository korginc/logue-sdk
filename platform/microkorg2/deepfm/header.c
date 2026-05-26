/**
 *  @file header.c
 *  @brief DeepFM — 2-operator FM oscillator for microKORG 2
 */

#include "unit.h"
#include "runtime.h"

#define NUM_RATIOS 16

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x44504D21U,      // "DPM!" — unique dev id
    .unit_id = 0x00000001U,
    .version = 0x00010000U,     // v1.0.0
    .name = "DeepFM",
    .num_presets = 0,
    .num_params = 10,
    .params = {
    // Page 1: Core FM controls (these 3 are also modulatable via virtual patch)
    //  min, max,           center, default, type,                      frac, frac_mode, reserved, name
    {0,   NUM_RATIOS-1,     0,      1,       k_unit_param_type_strings, 0,    0,         0,        {"RATIO"}},
    {0,   1000,             0,      0,       k_unit_param_type_percent, 1,    1,         0,        {"FM DPTH"}},
    {0,   1000,             0,      0,       k_unit_param_type_percent, 1,    1,         0,        {"FEEDBCK"}},

    // Page 2: Envelope and tone shaping
    {0,   1000,             0,      0,       k_unit_param_type_percent, 1,    1,         0,        {"FM DECY"}},
    {0,   1000,             500,    500,     k_unit_param_type_none,    1,    1,         0,        {"FINE"}},
    {0,   1000,             0,      0,       k_unit_param_type_percent, 1,    1,         0,        {"SUB LVL"}},
    {0,   1000,             0,      0,       k_unit_param_type_percent, 1,    1,         0,        {"WARMTH"}},
    {0,   1000,             0,      0,       k_unit_param_type_percent, 1,    1,         0,        {"STEREO"}},

    // Page 3: Extra shaping
    {0,   1000,             0,      0,       k_unit_param_type_percent, 1,    1,         0,        {"ATTACK"}},
    {0,   1000,             0,      0,       k_unit_param_type_percent, 1,    1,         0,        {"HARMNCS"}},

    // Unused slots
    {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
    {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
};
