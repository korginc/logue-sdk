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
    .num_params  = 14,
    .params = {
        // Page 1

        // ID 0: NAME  string
        { 0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"NAME"} },
        // ID 1: DARK  sub octave  0%-100%
        { 0, 100, 10, 60, k_unit_param_type_percent, 0, 0, 0, {"DARK"} },
        // ID 2: BRIG  brightness (high-freq level)  0%-100%
        { 0, 100, 10, 50, k_unit_param_type_percent, 0, 0, 0, {"BRIG"} },
        // ID 3: GLOW  modulation  0%-100%
        { 0, 100, 10, 70, k_unit_param_type_percent, 0, 0, 0, {"GLOW"} },

        // Page 2
        // ID 4: COLR  tone color (spectrum resonance)  0%-100%
        { 0, 100, 10, 10, k_unit_param_type_percent, 0, 0, 0, {"COLR"} },
        // ID 5: SPRK  sparkle / modulation depth  0%-100%
        { 0, 100, 10, 5, k_unit_param_type_percent, 0, 0, 0, {"SPRK"} },
        // ID 6: SIZE  room size (delay scale)  0%-100%
        { 0, 100, 10, 50, k_unit_param_type_percent, 0, 0, 0, {"SIZE"} },
        // ID 7: PDLY  pre-delay time 0-100% (mapped to 0-200ms)
        { 0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"PDLY"} },

        // Page 3
        // ID 8: DCAY  FDN feedback (decay length)  0%-100%
        { 0, 100, 50, 65, k_unit_param_type_percent, 0, 0, 0, {"DCAY"} },
        // ID 9: BASS  per-channel HPF in FDN loop (bass cut in tail)  0%=flat 100%=max cut
        { 0, 100, 30, 30, k_unit_param_type_percent, 0, 0, 0, {"BASS"} },
        // ID 10: CLRQ  tone color shift  -100%-100%
        { -100, 100, 10, 10, k_unit_param_type_percent, 0, 0, 0, {"CLRQ"} },
        // ID 11: RATE  glow LFO speed  0%-100% (mapped to 0.05-4.0 Hz exponential)
        { 0, 100, 0, 20, k_unit_param_type_percent, 0, 0, 0, {"RATE"} },

        // Page 4
        // ID 12: IRID  iridiscence (saturated optical halo)  0%-100%
        { 0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"IRID"} },
        // ID 13: WDTH  stereo width  0%-100% (0=mono, 50=unity, 100=extra wide)
        { 0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"WDTH"} },

        // Pages 4-6: blank padding
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
