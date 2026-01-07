/**
 *  @file header.c
 *  @brief drumlogue SDK unit header
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"  // Note: Include common definitions for all units

// ---- Unit header definition  --------------------------------------------------------------------
/* FOR PORTING see RipplerXAudioProcessor::onSlider()
in this case we do not have sliders but menu pages with parameters*/
const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),                  // leave as is, size of this header
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,  // target platform and module for this unit
    .api = UNIT_API_VERSION,                               // logue sdk API version against which unit was built
    .dev_id = 0x46654465U,                                 // 'FeDe' - https://github.com/fedemone/logue-sdk
    .unit_id = 0x5265736fU,                                // TODO - Id for this unit, should be unique within the scope of a given dev_id
    .version = 0x00010000U,                                // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "RipplerX",                                    // Name for this unit, will be displayed on device
    .num_presets = 28,                                     // Number of internal presets this unit has
    .num_params = 24,                                      // Number of parameters for this unit, max 24
    .params = {
        // Format: min, max, center, default, type, fractional digits, frac. type (fixed/decimal), <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        //FOR PORTING. On page one must be synth type and load from xml the parameters accordingly
        //             on other pages, all the "PARAMETERS" (see PluginPorcessor.cpp)
        //             see also unit_load_preset

        // Page 1: Program and sample selection
        // Program, will set different values for parameters - TODO - check this!!!
        {0, 27, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Program"}},
        // Res Gain
        {-240, 240, 0, 0, k_unit_param_type_none, 1, 0, 0, {"Gain"}},
        // Sample bank
        {0, 6, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Bank"}},
        // Sample number
        {1, 128, 1, 1, k_unit_param_type_none, 0, 0, 0, {"Sample"}},

        // Page 2: Mallet
        // Mallet resonance
        {0, 10, 0, 8, k_unit_param_type_none, 1, 1, 0, {"Mallet Res"}},
        // Mallet stiffness
        {100, 5000, 2560, 600, k_unit_param_type_none, 0, 0, 0, {"Mallet Stiff"}},
        // Velocity Mallet Resonance
        {0, 1000, 0, 0, k_unit_param_type_none, 3, 0, 0, {"Vel Mal Res"}},
        // Velocity Mallet Stiffness
        {0, 1000, 0, 0, k_unit_param_type_none, 3, 1, 0, {"Vel Mal Stif"}},

        // Page 3: Resonator A-I (B is switched off to lack of parameter pages to handle)
        //  Model - "String", "Beam", "Squared", "Membrane", "Plate", "Drumhead", "Marimba", "Open Tube", "Closed Tube"
        {0, 8, 0, 3, k_unit_param_type_strings, 0, 0, 0, {"Model"}},
        //  Partials -  "4", "8", "16", "32", "64"
        {0, 4, 0, 3, k_unit_param_type_strings, 0, 0, 0, {"Partials"}},
        // Decay
        {0, 1000, 520, 10, k_unit_param_type_none, 1, 1, 0, {"Decay"}},
        // Material (-1.0, 1.0)
        {-10, 10, 0, 0, k_unit_param_type_none, 1, 0, 0, {"Material"}},

        // TODO: use ratio in order to update the model?

        // Page 4: Resonator A-II
        // Tone (-1.0, 1.0)
        {-10, 10, 0, 0, k_unit_param_type_none, 1, 0, 0, {"Tone"}},
        // Hit Position (0.02, 0.26)
        {2, 50, 0, 26, k_unit_param_type_none, 2, 0, 0, {"HitPos"}},
        // Release
        {0, 10, 0, 10, k_unit_param_type_none, 1, 0, 0, {"Release"}},
        // Inharmonic
        {1, 10000, 3000, 1, k_unit_param_type_none, 4, 1, 0, {"Inharmonic"}},

        // Page 5: Resonator A-III + Noise-I
        // filter cutoff
        {20, 20000, 10010, 20, k_unit_param_type_hertz, 0, 0, 0, {"LowCut"}},
        // Tube Radius
        {0, 10, 0, 5, k_unit_param_type_none, 1, 0, 0, {"Tube Radius"}},
        // Coarse Pitch
        {-480, 480, 0, 0, k_unit_param_type_none, 0, 0, 0, {"Coarse Pitch"}},
        // Noise Mix
        {0, 1000, 300, 0, k_unit_param_type_percent, 1, 1, 0, {"Noise Mix"}},
        //TODO: vel_noise_freq and vel_noise_q should be here?

        // Page 6: Noise II
        // Noise Resonance
        {0, 1000, 300, 0, k_unit_param_type_percent, 1, 1, 0, {"Noise Res"}},
        // Noise Filter Mode: "LP", "BP", "HP"
        {0, 2, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"Noise Filter"}},
        // Noise Filter Freq
        {20, 20000, 12000, 20, k_unit_param_type_hertz, 0, 0, 0, {"NoiseFiltFrq"}},
        // Noise Filter Q
        {707, 4000, 0, 707, k_unit_param_type_none, 3, 1, 0, {"NoiseFilterQ"}}
        }   // end of .params
    };

