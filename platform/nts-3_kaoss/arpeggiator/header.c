/*
 *  File: header.c
 *
 *  NTS-3 kaoss pad kit arpeggiator unit header definition
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
            .unit_id = 0x02U,
            .version = 0x00010000U,
            .name = "Arpeggiator",
            .num_params = 8,

            .params =
                {// 0: ROOT (X mapping)
                 {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"ROOT"}},

                 // 1: CHORD (Y mapping)
                 {0, 10, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"CHORD"}},

                 // 2: GATE (DEPTH mapping)
                 {0, 1023, 512, 512, k_unit_param_type_none, 0, 0, 0, {"GATE"}},

                 // 3: PATTERN
                 {0, 7, 2, 2, k_unit_param_type_strings, 0, 0, 0, {"PATTERN"}},

                 // 4: MODE
                 {0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"MODE"}},

                 // 5: RANGE
                 {1, 4, 1, 1, k_unit_param_type_none, 0, 0, 0, {"RANGE"}},

                 // 6: WAVE
                 {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"WAVE"}},

                 // 7: LEVEL
                 {0, 1023, 512, 512, k_unit_param_type_none, 0, 0, 0, {"LEVEL"}}},
        },
    .default_mappings = {
        // ROOT mapped to X axis
        {k_genericfx_param_assign_x, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 0},

        // CHORD mapped to Y axis
        {k_genericfx_param_assign_y, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 10, 0},

        // GATE (unmapped, edit via menu)
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 512},

        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 7, 2},

        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 4, 2},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 4, 2},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 2, 1},
         
        // LEVEL mapped to DEPTH control
        {k_genericfx_param_assign_depth, k_genericfx_curve_linear,
         k_genericfx_curve_unipolar, 0, 1023, 512}}};
