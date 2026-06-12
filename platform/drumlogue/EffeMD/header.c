/**
 * @file header.c
 * @brief FM Percussion Synth - One instance per instrument
 */

#include "unit.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,
    .api = UNIT_API_VERSION,
    .dev_id = 0x46654465U,   // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id = 0x33U,        //  unique unit_id
    .version = 0x00010000U,  // v1.0.0 — required; version=0 causes load rejection on drumlogue
    .name = "EffeMD",
    .num_presets = 2,       // presets should be different from instrument model. We have default and a set, but possible to be addedd in the future.
    .num_params = 24,
    .params = {
        // TODO increase range to 1000? in case divide by ten all setParameter assignments
        // will be displayed a O:.%f if parameter is valid for instrument, X otherwise. Disabling value shall be 255, not assignable by user.
        // each instrument will be stored in a local array of 24 parameters, and only valid ones will be updated by the UI and processed by the DSP. This allows for a flexible design where different instruments can have different sets of parameters while sharing the same underlying structure.
        // Page 1:
        // Instrument
        {0, 10, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Instr"}},  // aligned to INST_COUNT
        // Base Frequency: fundamental frequency of the FM
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"BasFrq"}},
        // Modulation Frequency: rate of modulation for the FM
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"ModFrq"}},
        // Modulation Index: depth of modulation for the FM
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"ModIdx"}},  // can be ratio index or absolute modulation index depending on UseRatio

        // Page 2:
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"ModDk"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"ModFdk"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"DkA"}},
        {0, 1, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"UseRatio"}},

        // Page 3:
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"Mix"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"HPF"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"LPF"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"Sustain"}},

        // Page 4:
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"FrqSwp"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"SwpDk"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"NzLvl"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"NzDk"}},

        // Page 5:
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"MdFrqB"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"DkB"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"MdIdxB"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"Phase"}},

        // Page 6:
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"Gap"}},
        {1, 6, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"Count"}},
        {0, 100, 0, 50, k_unit_param_type_strings, 0, 0, 0, {"-"}},  // TODO reserved for future use
        // EuclTun: Euclidean per-voice pitch spread. Assigns voice i a semitone
        //   offset = floor(i * n / 4) so the 4 voices are maximally spread across
        //   n chromatic steps. Mode 0 = off (all voices same pitch).
        {0, 8, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"EuclTun"}},
    }
};
