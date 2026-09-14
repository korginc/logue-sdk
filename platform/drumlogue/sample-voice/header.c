/**
 *  @file header.c
 *  @brief drumlogue SDK unit header for "SmplVox" sample-playback drum voice
 *
 *  Demonstrates capabilities unique to the drumlogue platform:
 *    - browsing and playing back the device's on-board PCM sample banks via the
 *      runtime descriptor (get_num_sample_banks / get_num_samples_for_bank /
 *      get_sample), something no other logue-SDK synth target exposes,
 *    - gate-based drum triggering with per-hit velocity,
 *    - classic drum-synthesis shaping (pitch sweep, decay envelope, lo-fi
 *      bit/rate reduction, drive) layered on top of arbitrary samples.
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"  // Note: Include common definitions for all units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),                  // leave as is, size of this header
    .target = UNIT_TARGET_PLATFORM | k_unit_module_synth,  // target platform and module for this unit
    .api = UNIT_API_VERSION,                               // logue sdk API version against which unit was built
    .dev_id = 0x0U,                                        // developer identifier
    .unit_id = 0x53564F58U,                                // "SVOX" - unique within the scope of dev_id
    .version = 0x00010000U,                                // 1.0.0
    .name = "SmplVox",                                     // Name for this unit, will be displayed on device
    .num_presets = 0,                                      // Number of internal presets this unit has
    .num_params = 12,                                      // Number of parameters for this unit, max 24
    .params = {
        // Format: min, max, center, default, type, frac, frac. mode, <reserved>, name

        // Page 1 -- sample selection & tuning
        // Bank index into the device sample banks. String value resolves to "BANK n".
        {0, 31, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"BANK"}},
        // Sample index within the selected bank. String value resolves to the sample's name.
        {0, 127, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"SAMPLE"}},
        // Coarse pitch in semitones.
        {-24, 24, 0, 0, k_unit_param_type_semi, 0, 0, 0, {"PITCH"}},
        // Fine pitch in cents.
        {-100, 100, 0, 0, k_unit_param_type_cents, 0, 0, 0, {"FINE"}},

        // Page 2 -- amp & pitch envelopes
        // Amplitude decay time, percent mapped to ~2ms..4s.
        {0, (100 << 1), 0, (60 << 1), k_unit_param_type_percent, 1, 0, 0, {"DECAY"}},
        // Pitch-sweep depth in semitones (bipolar): start pitch offset that decays to 0.
        {-48, 48, 0, 0, k_unit_param_type_semi, 0, 0, 0, {"P.SWEEP"}},
        // Pitch-sweep time, percent mapped to ~2ms..1s.
        {0, (100 << 1), 0, (20 << 1), k_unit_param_type_percent, 1, 0, 0, {"P.TIME"}},
        // Sample start offset, percent of the sample length.
        {0, (100 << 1), 0, 0, k_unit_param_type_percent, 1, 0, 0, {"START"}},

        // Page 3 -- character & output
        // Reverse playback.
        {0, 1, 0, 0, k_unit_param_type_onoff, 0, 0, 0, {"REVERSE"}},
        // Bit-depth reduction amount.
        {0, (100 << 1), 0, 0, k_unit_param_type_percent, 1, 0, 0, {"CRUSH"}},
        // Saturation / drive amount.
        {0, (100 << 1), 0, 0, k_unit_param_type_percent, 1, 0, 0, {"DRIVE"}},
        // Output pan.
        {-100, 100, 0, 0, k_unit_param_type_pan, 0, 0, 0, {"PAN"}},

        // Page 4
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        // Page 5
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

        // Page 6
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}}};
