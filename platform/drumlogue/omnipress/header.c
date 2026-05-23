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
    .num_params  = 24,

    .params = {
        // Page 1: Core Dynamics
        // ID 0: THRESH  -60.0..0.0 dB  (x0.1, stored -600..0)
        { -600, 0, -600, -200, k_unit_param_type_db, 1, 1, 0, {"THRESH"} },
        // ID 1: SLOPE   1.0:1..10.0:1
        // 0.00 -> 0.33 : Expansion (Slope +3.0 down to 0.0)
        // 0.33 -> 0.66 : Compression (Slope 0.0 down to -1.0 limit)
        // 0.66 -> 1.00 : Negative Compression (Slope -1.0 down to -2.0 reverse)
        { 1, 100, 10, 40, k_unit_param_type_none, 2, 1, 0, {"SLOPE"} },
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
        { 0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"BASS"} },     // ID 9  - Overlord
        { 0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"TREBLE"} },   // ID 10 - Overlord
        { 0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"PRESENCE"} }, // ID 11 - Overlord

        // Page 4: Multiband / Distressor Parameters
        // ID 12: DSTR DIST  0=None, 1=2nd harm, 2=3rd harm, 3=Both, 4=Wave
        { 0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"DstrDIST"} },    // ID 12 - Distressor distortion type
        { 0, 7, 0, 3, k_unit_param_type_strings, 0, 0, 0, {"DstrRATIO"} },   // ID 13 - Distressor ratio selection - TODO - remove this in favoir of atten_limit_ param and either deduce from threshold, or move inside distressor params since it only applies to distressor mode, and rename to "Drive Type" or similar, since it only applies to distressor mode)
        // ID 14: DSTR WAVE  0=Soft, 1=Hard, 2=Tri, 3=Sine, 4=SubOct
        { 0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"DstrWAVE"} },    // ID 14 - Wavefolder drive type - todo remove this (use distortion type and get new gain_limit_, Still need atten_limit_ param for opto mode though, so maybe just move inside distressor params and rename to "Drive Type" or similar, since it only applies to distressor mode)
        // ID 15: BAND SEL  0=Low, 1=Mid, 2=High, 3=Low+Mid, 4=Low+High, 5=Mid+High, 6=All
        { 0, 6, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"MBand"} },       // ID 15

        // Page 5: Multiband per-band parameters
        // ID 16: BAND THRESH  -60.0..0.0 dB  (per-band threshold)
        { -600, 0, -600, -200, k_unit_param_type_strings, 1, 1, 0, {"MBndThr"} },  // dB
        // ID 17: BAND RATIO   1.0..20.0:1
        { 10, 200, 10, 40, k_unit_param_type_strings, 1, 1, 0, {"MBndRto"} },      // ID 17 - ratio (param none)
        // ID 18: BAND ATTACK  1..1000 ms
        { 1, 1000, 1, 150, k_unit_param_type_strings, 1, 1, 0, {"MBndAtk"} },      // ID 18 - ms
        // ID 19: BAND RELEASE 10..2000 ms
        { 10, 2000, 10, 200, k_unit_param_type_strings, 0, 0, 0, {"MBndRtoRel"} }, // ID 19 - ms

        // Page 6:
        { 0, 240, 0, 0, k_unit_param_type_strings, 1, 1, 0, {"MBndMkp"} },         // ID 20 - dB
        { 0, 1, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"MBndMut"} },           // ID 21 (param none) - Mute (0=off, 1=on)
        { 0, 1, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"MBndSOl"} },           // ID 22 (param none) - Solo (0=off, 1=on)
        // ID 23: DETECT MODE
        //   Standard/Multiband: 0=Peak,  1=RMS,  2=Blend
        //   Distressor:         0=Basic, 1=Emph, 2=Link, 3=Emph+Link
        { 0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"DtctMOD"} },        // ID 23
    }
};
