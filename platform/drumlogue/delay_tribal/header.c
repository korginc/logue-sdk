/**
 *  @file header.c
 *  @brief Percussion Spatializer for drumlogue
 *
 *  Transforms single hits into ensemble performances
 *  Version 1.1 - Added preset parameter
 */

#include "unit.h"
#include "constants.h"
#include "presets.h"

/**
 * Parameter Map (7 parameters, expandable to 24):
 *
 * Page 1 - Core Controls
 * PARAM1: Clone Count - Number of simulated instruments (4, 8, 16)
 * PARAM2: Mode - Tribal (circular), Military (linear), Angel (stochastic)
 * PARAM3: Depth - Intensity of spatial effect (0-100%)
 *
 * Page 2 - Modulation & Timbre
 * PARAM4: Filter Freq - Center/cutoff frequency (20 Hz - 8 kHz)
 * PARAM5: Filter Q - Resonance/bandwidth (0.5 - 10)
 * PARAM6: Mix - Wet/dry balance (0-100%)
 *
 * Page 3 - Presets
 * PARAM7: Preset - Load factory/user preset (0-15)
 */
const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_delfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x46654465U,       // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id = 0x01U,
    .version = 0x00010001U,      // v1.1.0
    .name = "TribùArmata",
    .num_presets = NUM_PRESETS,  // 8 factory presets
    .num_params = 7,             // Now using 7 parameters

    .params = {
        // Page 1 - Core Controls
        {PARAM_CLONES_MIN, PARAM_CLONES_MAX, 0, 0,
         k_unit_param_type_strings, 0, 0, 0, {"Clones"}},

        {PARAM_MODE_MIN, PARAM_MODE_MAX, 0, 0,
         k_unit_param_type_strings, 0, 0, 0, {"Mode"}},

        {PARAM_DEPTH_MIN, PARAM_DEPTH_MAX, 0, 50,
         k_unit_param_type_percent, 0, 0, 0, {"Depth"}},

        // Page 2 - Timbre Controls
        {PARAM_FREQ_MIN, PARAM_FREQ_MAX, 0, 1000,
         k_unit_param_type_none, 0, 0, 0, {"Freq"}},

        {PARAM_Q_MIN, PARAM_Q_MAX, 0, 7,
         k_unit_param_type_none, 1, 0, 0, {"Q"}},

        {PARAM_MIX_MIN, PARAM_MIX_MAX, 0, 50,
         k_unit_param_type_percent, 0, 0, 0, {"Mix"}},

        // Page 3 - Presets
        {0, NUM_PRESETS + NUM_USER_PRESETS - 1, 0, 0,
         k_unit_param_type_strings, 0, 0, 0, {"Preset"}},

        // Remaining slots blank
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        // ... (continue for total 24 entries)
    }
};

/**
 * Get preset name for display
 * Called by the system when preset parameter is displayed
 */
const char* unit_get_preset_name(uint8_t idx) {
    return get_preset_name(idx);
}