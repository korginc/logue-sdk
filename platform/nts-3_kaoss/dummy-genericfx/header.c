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
 *  NTS-3 kaoss pad kit generic effect unit header definition
 *
 */

#include "unit_genericfx.h"   // Note: Include base definitions for genericfx units

// ---- Unit header definition  --------------------------------------------------------------------

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),           // Size of this header. Leave as is.
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx, // Target platform and module pair for this unit
    .api = UNIT_API_VERSION,                                  // API version for which unit was built. See runtime.h
    .dev_id = 0x0,                                            // Developer ID. See https://github.com/korginc/logue-sdk/blob/master/developer_ids.md
    .unit_id = 0x0U,                                          // ID for this unit. Scoped within the context of a given dev_id.
    .version = 0x00010000U,                                   // This unit's version: major.minor.patch (major<<16 minor<<8 patch).
    .name = "dummy",                                          // Name for this unit, will be displayed on device
    .num_params = 4,                                          // Number of valid parameter descriptors. (max. 8)
    
    .params = {
      // Format: min, max, center (unused), default, type, frac. bits, frac. mode, <reserved>, name
      
      // See common/runtime.h for type enum and unit_param_t structure

      // Examples of simple numeric parameters
      {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PARAM1"}},
      {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PARAM2"}},
      
      // Example of a parameter with negative values and one fractional digit (base 10), using the drywet display type 
      {-1000, 1000, 0, 0, k_unit_param_type_drywet, 1, 1, 0, {"DEPTH"}},

      // Example of a strings type parameter
      {0, 3, 0, 1, k_unit_param_type_strings, 0, 0, 0, {"PARAM4"}},
      
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}},
  },
  .default_mappings = {
    // By default, the parameters described above will be mapped to controls as described below.
    // These assignments can be overriden by the user.
    
    // Format: assign, curve, curve polarity, min, max, default value

    // PARAM1 mapped full range to X axis of control pad, initialized at 256
    {k_genericfx_param_assign_x, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 256},

    // PARAM2 mapped half range to Y axis of control pad, initialized at 512
    {k_genericfx_param_assign_y, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 512, 1023, 512},

    // DEPTH mapped full range to depth control, with a bipolar exponential curve and i initialized at 0
    {k_genericfx_param_assign_depth, k_genericfx_curve_exp, k_genericfx_curve_bipolar, -1000, 1000, 0},

    // PARAM4 set to the fixed value of 1
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 3, 1},
    
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
