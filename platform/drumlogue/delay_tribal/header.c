/**
 * @file header.c
 * @brief drumlogue SDK unit header for Enhanced Percussion Spatializer
 *
 * Now includes parameters for:
 * - Velocity Randomization (implied)
 * - Pitch Wobble Depth
 * - Attack Softening
 */

#include "unit.h"

// String tables for parameter display
static const char* mode_strings[3] = {
    "Tribal",
    "Military",
    "Angel"
};

static const char* clone_strings[3] = {
    "4",
    "8",
    "16"
};

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_delfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x46654465U,       // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id = 0x01U,
    .version = 0x00020000U,      // v2.0.0 with enhancements
    .name = "PercSpatial",
    .num_presets = 0,
    .num_params = 8,  // Now using 8 parameters

    .params = {
        // ---------- Page 1: Core Controls ----------
        // PARAM1: Clone Count (0=4, 1=8, 2=16)
        {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Clones"}},

        // PARAM2: Spatial Mode (0=Tribal, 1=Military, 2=Angel)
        {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Mode"}},

        // PARAM3: Effect Depth (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"Depth"}},

        // ---------- Page 2: Modulation ----------
        // PARAM4: Modulation Rate (0.1-10 Hz, scaled)
        {0, 100, 0, 30, k_unit_param_type_none, 1, 0, 0, {"Rate"}},

        // PARAM5: Stereo Spread (0-100%)
        {0, 100, 0, 80, k_unit_param_type_percent, 0, 0, 0, {"Spread"}},

        // PARAM6: Wet/Dry Mix (0-100%)
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"Mix"}},

        // ---------- Page 3: Ensemble Enhancements ----------
        // PARAM7: Pitch Wobble Depth (0-100%)
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"Wobble"}},

        // PARAM8: Attack Softening (0-100%)
        {0, 100, 0, 20, k_unit_param_type_percent, 0, 0, 0, {"SoftAtk"}},

        // Remaining slots blank (16 more to fill 24 total)
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        // ... (continue to 24)
    }
};

// String display callbacks
const char* unit_get_param_str_value(uint8_t index, int32_t value) {
    switch (index) {
        case 0: // Clones
            if (value >= 0 && value <= 2) return clone_strings[value];
            break;
        case 1: // Mode
            if (value >= 0 && value <= 2) return mode_strings[value];
            break;
    }
    return nullptr;
}