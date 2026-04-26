/*
 *  File: header.c
 *
 *  NTS-3 kaoss pad kit noise generator unit header definition
 *
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
    .common =
        {
            .header_size = sizeof(genericfx_unit_header_t),
            .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
            .api = UNIT_API_VERSION,
            .dev_id = 0x0, // Custom/Generic
            .unit_id = 0x01U,
            .version = 0x00010000U,
            .name = "Noise Gen",
            .num_params = 7,

            .params =
                // Format: min, max, center, default, type, frac. bits, frac.
                // mode, <reserved>, name
            {// LEVEL: overall gain
             {0, 1023, 0, 512, k_unit_param_type_none, 0, 0, 0, {"LEVEL"}},

             // PAN: stereo balance
             {0, 1023, 512, 512, k_unit_param_type_pan, 0, 0, 0, {"PAN"}},

             // TYPE: noise spectral type
             {0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"TYPE"}},

             // DRYWET: dry/wet mix
             {0,
              1023,
              1023,
              512,
              k_unit_param_type_drywet,
              0,
              0,
              0,
              {"DRYWET"}},

             // AUTOPAN: strength/depth
             {0, 1023, 0, 20, k_unit_param_type_none, 0, 0, 0, {"AUTOPAN"}},

             // APAN_SPD: speed/rate
             {0, 1023, 512, 512, k_unit_param_type_none, 0, 0, 0, {"APAN_SPD"}},

             // APAN_SYNC: sync option
             {0, 6, 0, 1, k_unit_param_type_strings, 0, 0, 0, {"APAN_SYNC"}},

             {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}},
        },
    .default_mappings = {
        // LEVEL mapped to Y axis
        {k_genericfx_param_assign_y, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 512},

        // PAN mapped to X axis
        {k_genericfx_param_assign_x, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 512},

        // TYPE mapped to DEPTH control
        {k_genericfx_param_assign_depth, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 4, 0},

        // DRYWET initialized at 1023 (100% wet)
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 512},

        // AUTOPAN default mapping
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 20},

        // APAN_SPD default mapping
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 60},

        // APAN_SYNC default mapping
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 6, 1},

        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 0, 0}}};
