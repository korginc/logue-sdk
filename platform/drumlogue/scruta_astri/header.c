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
    .num_presets = 16,
    .num_params = 24,
    .params = {
        // Page 1: Base
        {0, 15, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Program"}},
        {24, 126, 1, 36, k_unit_param_type_midi_note, 0, 0, 0, {"Note"}},
        {0, 89, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Osc1Wave"}},
        {0, 89, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Osc2Wave"}},

        // Page 2: Osc 2 Controls
        {-100, 100, 0, 0, k_unit_param_type_none, 0, 0, 0, {"O2Detune"}},
        {0, 1, 0, 0, k_unit_param_type_none, 0, 0, 0, {"O2SubOct"}},
        {0, 100, 50, 0, k_unit_param_type_percent, 0, 0, 0, {"Osc2Mix"}},
        {0, 100, 50, 0, k_unit_param_type_percent, 0, 0, 0, {"MastrVol"}},

        // Page 3: Filters
        {20, 15000, 10000, 0, k_unit_param_type_hertz, 0, 0, 0, {"F1Cutoff"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"F1Reso"}},
        {20, 15000, 10000, 0, k_unit_param_type_hertz, 0, 0, 0, {"F2Cutoff"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"F2Reso"}},

        // Page 4: LFO 1 & 2
        {0, 5, 0, 0, k_unit_param_type_none, 0, 0, 0, {"L1Wave"}},
        {1, 100, 10, 0, k_unit_param_type_none, 0, 0, 0, {"L1Rate"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"L1Depth"}},
        {1, 100, 10, 0, k_unit_param_type_none, 0, 0, 0, {"L2Rate"}},

        // Page 5: LFO 2 & 3
        {0, 5, 0, 0, k_unit_param_type_none, 0, 0, 0, {"L2Wave"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"L2Depth"}},
        {0, 5, 0, 0, k_unit_param_type_none, 0, 0, 0, {"L3Wave"}},
        {1, 100, 10, 0, k_unit_param_type_none, 0, 0, 0, {"L3Rate"}},

        // Page 6: FX & LoFi
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"L3Depth"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"SampRed"}},
        {0, 15, 0, 0, k_unit_param_type_none, 0, 0, 0, {"BitRed"}},
        {0, 100, 0, 0, k_unit_param_type_percent, 0, 0, 0, {"CMOSDist"}}
    }
};