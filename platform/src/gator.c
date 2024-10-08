/*
 *  File: gator.c
 *
 *  Gator 2.0 unit header
 *
 *  2024 (c) Oleg Burdaev
 *  mailto: dukesrg@gmail.com
 */

#include "logue_wrap.h"

const __unit_header UNIT_HEADER_TYPE unit_header = {
#ifdef UNIT_TARGET_PLATFORM_NTS3_KAOSS
    .common = {
#endif
    .header_size = sizeof(UNIT_HEADER_TYPE),
    .target = UNIT_HEADER_TARGET_VALUE,
    .api = UNIT_API_VERSION,
    .dev_id = 0x44756B65U,
    .unit_id = 0x32525447U,
    .version = 0x00020000U,
    .name = UNIT_NAME,
    .num_params = PARAM_COUNT,
    .params = {
#ifdef UNIT_TARGET_PLATFORM_NTS1_MKII
        {0, 1, 0, 0, k_unit_param_type_onoff, 0, k_unit_param_frac_mode_fixed, 0, {"Gate"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, k_unit_param_frac_mode_fixed, 0, {""}},
#if defined(UNIT_DELFX_H_) || defined(UNIT_REVFX_H_)
        {0, 0, 0, 0, k_unit_param_type_none, 0, k_unit_param_frac_mode_fixed, 0, {""}},
#endif 
#endif
        {0, 1, 0, 0, k_unit_param_type_onoff, 0, k_unit_param_frac_mode_fixed, 0, {"Gate"}},
        {0, 50, 0, 0, k_unit_param_type_none, 0, k_unit_param_frac_mode_fixed, 0, {"Pattern"}},
        {-1023, 0, 0, 0, k_unit_param_type_db, 1, k_unit_param_frac_mode_decimal, 0, {"Thresh"}},
        {0, 1023, 0, 0, k_unit_param_type_msec, 0, k_unit_param_frac_mode_decimal, 0, {"Time"}},
        {0, 1023, 0, 0, k_unit_param_type_msec, 0, k_unit_param_frac_mode_decimal, 0, {"Attack"}},
        {0, 1023, 0, 0, k_unit_param_type_msec, 0, k_unit_param_frac_mode_decimal, 0, {"Decay"}},
        {-1023, 0, 0, 0, k_unit_param_type_db, 1, k_unit_param_frac_mode_decimal, 0, {"Sustain"}},
        {0, 1023, 0, 0, k_unit_param_type_msec, 0, k_unit_param_frac_mode_decimal, 0, {"Release"}},
#if defined(UNIT_TARGET_PLATFORM_DRUMLOGUE) && defined(UNIT_TARGET_MODULE_MASTERFX)
        {0, 2, 0, 2, k_unit_param_type_strings, 0, k_unit_param_frac_mode_fixed, 0, {"Master"}},
        {0, 2, 0, 0, k_unit_param_type_strings, 0, k_unit_param_frac_mode_fixed, 0, {"Sidech"}},
        {0, 2, 0, 0, k_unit_param_type_strings, 0, k_unit_param_frac_mode_fixed, 0, {"Peak"}}
#endif
    }
#ifdef UNIT_TARGET_PLATFORM_NTS3_KAOSS
    },
    .default_mappings = {
        {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 50, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, -1023, 0, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0},
        {k_genericfx_param_assign_depth, k_genericfx_curve_linear, k_genericfx_curve_unipolar, -1023, 0, 0},
        {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 0}
    }
#endif
};
