/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 *  File: header.c
 *
 *  NTS-1 mkII oscillator unit header definition
 *
 */

#include "unit_osc.h" // Note: Include base definitions for osc units

#include "waves_common.h" // Include common definitions specific to waves oscillator

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),               // Size of this header. Leave as is.
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc, // Target platform and module pair for this unit
    .api = UNIT_API_VERSION,                            // API version for which unit was built. See runtime.h
    .dev_id = 0x4B4F5247U,                              // Developer ID. See https://github.com/korginc/logue-sdk/blob/master/developer_ids.md
    .unit_id = 0x050400U,                               // ID for this unit. Scoped within the context of a given dev_id.
    .version = 0x00020000U,                             // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "WAVES",                                    // Name for this unit, will be displayed on device
    .num_params = 8,                                    // Number of valid parameter descriptors. (max. 10)
    .params = {
        // Format:
        // min, max, center, default, type, frac. bits, frac. mode, <reserved>, name

        // See common/runtime.h for type enum and unit_param_t structure

        // Fixed/direct UI parameters
        // A knob
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"SHPE"}},

        // B knob
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"SUB"}},

        // 8 Edit menu parameters
        {0, WAVE_A_CNT - 1, 0, 0, k_unit_param_type_enum, 0, 0, 0, {"WAVE A"}},
        {0, WAVE_B_CNT - 1, 0, 0, k_unit_param_type_enum, 0, 0, 0, {"WAVE B"}},
        {0, SUB_WAVE_CNT - 1, 0, 0, k_unit_param_type_enum, 0, 0, 0, {"SUB WAVE"}},
        {0, 1000, 0, 0, k_unit_param_type_percent, 1, 1, 0, {"RING MIX"}},
        {0, 1000, 0, 0, k_unit_param_type_percent, 1, 1, 0, {"BIT CRUSH"}},
        {0, 1000, 0, 250, k_unit_param_type_percent, 1, 1, 0, {"DRIFT"}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}},
};
