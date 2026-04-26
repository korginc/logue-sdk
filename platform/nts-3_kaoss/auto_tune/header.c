/*
 *  File: header.c
 *
 *  NTS-3 kaoss pad kit Auto Tune unit header definition
 *
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
    .common =
        {
            .header_size = sizeof(genericfx_unit_header_t),
            .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
            .api = UNIT_API_VERSION,
            .dev_id = 0x0,
            .unit_id = 0x02U, // Distinct from noise-generator
            .version = 0x00010000U,
            .name = "Auto Tune",
            .num_params = 4,

            .params =
                {// 0: ROOT - sets the key/root note
                 {0, 11, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"ROOT"}},

                 // 1: SCALE - selects the musical scale
                 {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"SCALE"}},

                 // 2: SPEED - correction speed (0: instant, 1023: slow)
                 {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"SPEED"}},

                 // 3: MIX - Dry/Wet balance
                 {0,
                  1023,
                  100,
                  512,
                  k_unit_param_type_percent,
                  0,
                  0,
                  0,
                  {"MIX"}},

                 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
                 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
                 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
                 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}},
        },
    .default_mappings = {
        // ROOT mapped to X axis
        {k_genericfx_param_assign_x, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 11, 0},

        // SCALE mapped to Y axis
        {k_genericfx_param_assign_y, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 8, 0},

        // SPEED mapped to DEPTH control
        {k_genericfx_param_assign_depth, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 0},

        // MIX mapped to parameter 4
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 512},

        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 0, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 0, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 0, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 0, 0}}};
