/**
 * @file header.c
 * @brief FM Percussion Synth with Voice Allocation
 *
 * Page 6 layout:
 * Param 20: EnvShape (0-127)
 * Param 21: VoiceAlloc (0-4) - Which instrument gets resonant treatment
 * Param 22: ResMode (0-4) - Resonant mode (LP/BP/HP/Notch/Peak)
 * Param 23: ResFreq (0-100%) - Resonant center frequency
 */

#include "unit.h"
#include "constants.h"

// String tables for parameter display
static const char* lfo_shape_strings[9] = {
    "Tri+Tri", "Ramp+Ramp", "Chord+Chord",
    "Tri+Ramp", "Tri+Chord", "Ramp+Tri",
    "Ramp+Chord", "Chord+Tri", "Chord+Ramp"
};

static const char* voice_mask_strings[15] = {
    "Kick", "Snare", "Metal", "Perc",
    "K+S", "K+M", "K+P", "S+M",
    "S+P", "M+P", "K+S+M", "K+S+P",
    "K+M+P", "S+M+P", "All"
};

static const char* resonant_mode_strings[5] = {
    "LowPass", "BandPass", "HighPass", "Notch", "Peak"
};

// Voice allocation strings (which instrument gets resonant treatment)
static const char* voice_alloc_strings[5] = {
    "K-S-M-P",  // 0: Original (all standard)
    "K-S-M-R",  // 1: Perc -> Resonant
    "K-S-R-P",  // 2: Metal -> Resonant
    "K-R-M-P",  // 3: Snare -> Resonant
    "R-S-M-P"   // 4: Kick -> Resonant
};

// LFO target strings (updated for resonant)
static const char* lfo_target_strings[8] = {
    "None",          // 0
    "Pitch",         // 1
    "Mod Index",     // 2
    "Envelope",      // 3
    "LFO2 Phase",    // 4
    "LFO1 Phase",    // 5
    "Res Freq",      // 6 - NEW: Resonant center frequency
    "Resonance"      // 7 - NEW: Resonance amount
};

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_delfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x7D0U,
    .unit_id = 0x02U,
    .version = 0x00020000U,      // v2.0.0 with resonant mode
    .name = "FMPerc",
    .num_presets = 12,            // 8 original + 4 resonant
    .num_params = 24,

    .params = {
        // ---------- Page 1: Trigger Probabilities ----------
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"ProbK"}},
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"ProbS"}},
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"ProbM"}},
        {0, 100, 0, 100, k_unit_param_type_percent, 0, 0, 0, {"ProbP"}},

        // ---------- Page 2: Kick + Snare Parameters ----------
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KSweep"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"KDecay"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"SNoise"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"SBody"}},

        // ---------- Page 3: Metal + Perc Parameters ----------
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"MInharm"}},
        {0, 100, 0, 70, k_unit_param_type_percent, 0, 0, 0, {"MBrght"}},
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"PRatio"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"PVar"}},

        // ---------- Page 4: LFO1 ----------
        {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L1Shape"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"L1Rate"}},
        {0, 7, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L1Dest"}},  // Updated range 0-7
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"L1Depth"}},

        // ---------- Page 5: LFO2 ----------
        {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L2Shape"}},
        {0, 100, 0, 30, k_unit_param_type_percent, 0, 0, 0, {"L2Rate"}},
        {0, 7, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"L2Dest"}},  // Updated range 0-7
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"L2Depth"}},

        // ---------- Page 6: Envelope + Voice Allocation + Resonant ----------
        {0, 127, 0, 40, k_unit_param_type_none, 0, 0, 0, {"EnvShape"}},
        {0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"VoiceAlloc"}},  // Param 21
        {0, 4, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"ResMode"}},     // Param 22
        {0, 100, 0, 50, k_unit_param_type_percent, 0, 0, 0, {"ResFreq"}}   // Param 23
    }
};

// String display callbacks
const char* unit_get_param_str_value(uint8_t index, int32_t value) {
    switch (index) {
        case 12: case 16:  // LFO1 Shape and LFO2 Shape
            if (value >= 0 && value <= 8) return lfo_shape_strings[value];
            break;
        case 14: case 18:  // LFO1 Dest and LFO2 Dest (updated range 0-7)
            if (value >= 0 && value <= 7) return lfo_target_strings[value];
            break;
        case 21:  // Voice Allocation
            if (value >= 0 && value <= 4) return voice_alloc_strings[value];
            break;
        case 22:  // Resonant Mode
            if (value >= 0 && value <= 4) return resonant_mode_strings[value];
            break;
    }
    return nullptr;
}