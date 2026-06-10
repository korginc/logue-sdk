/**
 * @file header.c
 * @brief drumlogue SDK unit header for PortaCassette Emulator
 */

#include "unit.h"

// Struct layout: min, max, center, init, type, frac, frac_mode, reserved, name
const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target      = UNIT_TARGET_PLATFORM | k_unit_module_masterfx, // Master bus effect
    .api         = UNIT_API_VERSION,
    .dev_id      = 0x46654465U,   // 'FeDe'
    .unit_id     = 0x506F7274U,   // 'Port'
    .version     = 0x00010000U,   // v1.0.0
    .name        = "PortaK7",     // 8 chars max for OLED
    .num_presets = 0,             // Can add presets later
    .num_params  = 14,            // 3 full pages

    .params = {
        // ==========================================
        // PAGE 1: Tape Mechanics & Gain Staging
        // ==========================================
        // ID 0: Tape Age (Wow/Flutter & high cut) (0 to 100%)
        { 0, 100, 0, 10, k_unit_param_type_percent, 0, 0, 0, {"Age"} },

        // ID 1: Mix (0 to 100%)
        { 0, 100, 50, 100, k_unit_param_type_percent, 0, 0, 0, {"Mix"} },

        // ID 2: Pre-Amp Gain (0 to 100%) - Pushes the input before the EQ
        { 0, 100, 0, 20, k_unit_param_type_percent, 0, 0, 0, {"PreAmp"} },

        // ID 3: Tape Drive (0 to 100%) - Pushes signal hard into the magnetic tape
        { 0, 100, 0, 40, k_unit_param_type_percent, 0, 0, 0, {"Drive"} },

        // ==========================================
        // PAGE 2: Parametric EQ (Low / Mid)
        // ==========================================
        // ID 4: dbx Mode (0 = Active, 1 = Encode Only (Bypass Decode), 2 = Off)
        { 0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"dbx NR"} },

        // ID 5: Model (0 = 244, 1 = 424, 2 = 488, 3 = Vinyl) - homage to old Tascam Portastudio
        { 0, 3, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Model"} },

        // ID 6: Low Frequency (2 = 20Hz, 40 = 400Hz -> x10 mapping)
        { 2, 40, 10, 8, k_unit_param_type_strings, 0, 0, 0, {"LowHz"} },

        // ID 7: Low Gain (-12dB to +12dB mapped as 0 to 240, 120 is 0dB)
        { 0, 240, 120, 120, k_unit_param_type_strings, 0, 0, 0, {"LowGain"} },

        // ==========================================
        // PAGE 3: Parametric EQ (High) & Output
        // ==========================================
        // ID 8: Mid Frequency (20 = 200Hz, 500 = 5000Hz -> x10 mapping)
        { 20, 500, 100, 100, k_unit_param_type_strings, 0, 0, 0, {"MidHz"} },

        // ID 9: Mid Gain (-12dB to +12dB mapped as 0 to 240, 120 is 0dB)
        { 0, 240, 120, 120, k_unit_param_type_strings, 0, 0, 0, {"MidGain"} },

        // ID 10: High Frequency (20 = 2000Hz, 150 = 15000Hz -> x100 mapping)
        { 20, 150, 80, 100, k_unit_param_type_strings, 0, 0, 0, {"HiHz"} },

        // ID 11: High Gain (-12dB to +12dB mapped as 0 to 240, 120 is 0dB)
        { 0, 240, 120, 120, k_unit_param_type_strings, 0, 0, 0, {"HiGain"} },


        // Pages 4-6: blank
        // ID 12: Crosstalk (mapped to 0.0f to 0.1f)
        { 0, 10, 0, 5, k_unit_param_type_none, 0, 0, 0, {"X-talk"} },
        // ID 13: Tape Bias Hiss (0 to 100%)
        { 0, 100, 0, 15, k_unit_param_type_percent, 0, 0, 0, {"Hiss"} },
        { 1, 20, 8, 10, k_unit_param_type_none, 2, 1, 0, {"Attack"} },
        { 1, 100, 5, 50, k_unit_param_type_none, 4, 1, 0, {"Release"} },

        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },

        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
        { 0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""} },
    }
};