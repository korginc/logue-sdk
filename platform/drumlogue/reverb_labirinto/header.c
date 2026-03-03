/**
 * @file header.c
 * @brief drumlogue SDK unit header for NeonLabirinto
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_revfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x00000000U,
    .unit_id = 0x00010000U,
    .version = 0x00010000U,       // v1.0.0
    .name = "NeonLabirinto",      // Max 13 ASCII characters
    .num_presets = 0,
    .num_params = 8,              // Exposing 8 parameters across 2 pages
    .params = {
        // Page 1: Main Reverb Controls
        // ID 0: MIX (0.0% to 100.0%)
        { .id = 0, .name = "MIX",  .min = 0, .max = 1000, .center = 500,  .default = 300, .type = k_unit_param_type_percent },
        // ID 1: TIME (Mid RT60: 0.1s to 10.0s)
        { .id = 1, .name = "TIME", .min = 1, .max = 100,  .center = 20,   .default = 20,  .type = k_unit_param_type_none },
        // ID 2: LOW (Low RT60: 0.1s to 10.0s)
        { .id = 2, .name = "LOW",  .min = 1, .max = 100,  .center = 20,   .default = 20,  .type = k_unit_param_type_none },
        // ID 3: HIGH (High RT60: 0.1s to 10.0s)
        { .id = 3, .name = "HIGH", .min = 1, .max = 100,  .center = 20,   .default = 10,  .type = k_unit_param_type_none },

        // Page 2: Advanced Network Controls
        // ID 4: DAMP (Crossover Freq: 200Hz to 10000Hz)
        { .id = 4, .name = "DAMP", .min = 200, .max = 10000, .center = 2500, .default = 2500, .type = k_unit_param_type_none },
        // ID 5: WIDE (Stereo Width: 0% to 200%)
        { .id = 5, .name = "WIDE", .min = 0, .max = 200,   .center = 100,  .default = 100,  .type = k_unit_param_type_percent },
        // ID 6: COMP (Complexity/Wall Rotation: 0% to 100%)
        { .id = 6, .name = "COMP", .min = 0, .max = 1000,  .center = 500,  .default = 1000, .type = k_unit_param_type_percent },
        // ID 7: PILL (Pillars Count: 0=4, 1=8, 2=16, 3=32)
        { .id = 7, .name = "PILL", .min = 0, .max = 3,     .center = 3,    .default = 3,    .type = k_unit_param_type_none },

        // Pad the rest of the 24 slots with empty definitions
        {0,0,0,0,k_unit_param_type_none,0,0,0,{""}}, {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
        {0,0,0,0,k_unit_param_type_none,0,0,0,{""}}, {0,0,0,0,k_unit_param_type_none,0,0,0,{""}},
        // ... (remaining 12 padded slots omitted for brevity, keep them all 0/none)
    }
};