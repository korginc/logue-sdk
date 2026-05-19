/**
 * @file header.c
 * @brief ScrutaAstri unit header
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x46654465U,       // 'FeDe'
    .unit_id = 0x53637275U,      // 'Scru'
    .version = 0x00010000U,
    .name = "ScrtAstr",          // Max 8 chars on OLED
    .num_presets = 95,
    .num_params = 24,
    .params = {
        // Page 1: Base
        {0, 95, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Prgrm"}},
        {24, 126, 1, 36, k_unit_param_type_midi_note, 0, 0, 0, {"Note"}},
        {0, 239, 0, 0, k_unit_param_type_none, 0, 0, 0, {"O1Wave"}},
        {0, 239, 0, 0, k_unit_param_type_none, 0, 0, 0, {"O2Wave"}},

        // Page 2: Osc 2 & Mix
        {-100, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"O2Dtun"}},
        {0, 3, 0, 0, k_unit_param_type_none, 0, 0, 0, {"O2Sub"}}, // 0..3 Range!
        {0, 100, 50, 50, k_unit_param_type_percent, 0, 0, 0, {"O2Mix"}},
        {0, 100, 50, 80, k_unit_param_type_percent, 0, 0, 0, {"Volume"}},

        // Page 3: Filters (2 to 1500 -> will be *10 for 20Hz-15000Hz)
        {2, 1500, 290, 80, k_unit_param_type_strings, 0, 0, 0, {"F1Cut"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"F1Res"}},
        {2, 1500, 310, 80, k_unit_param_type_strings, 0, 0, 0, {"F2Cut"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"F2Res"}},

        // Page 4: LFO 1 & 2
        {0, 8, 0, 0, k_unit_param_type_none, 0, 0, 0, {"L1Wave"}},
        {0, 100, 10, 10, k_unit_param_type_none, 0, 0, 0, {"L1Rate"}},
        {0, 100, 20, 0, k_unit_param_type_percent, 0, 0, 0, {"L1Dpth"}},
        {0, 8, 0, 0, k_unit_param_type_none, 0, 0, 0, {"L2Wave"}},

        // Page 5: LFO 2 & 3
        {0, 100, 10, 10, k_unit_param_type_none, 0, 0, 0, {"L2Rate"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"L2Dpth"}},
        {0, 8, 0, 0, k_unit_param_type_none, 0, 0, 0, {"L3Wave"}},
        {0, 100, 10, 70, k_unit_param_type_none, 0, 0, 0, {"L3Rate"}},

        // Page 6: FX
        {0, 100, 20, 0, k_unit_param_type_percent, 0, 0, 0, {"L3Dpth"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"SRRed"}},
        {0, 15, 0, 0, k_unit_param_type_none, 0, 0, 0, {"BitRed"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"CMOS"}}
    }
};
