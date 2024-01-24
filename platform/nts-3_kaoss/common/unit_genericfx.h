/*
    BSD 3-Clause License

    Copyright (c) 2018-2023, KORG INC.
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

/**
 *  @file unit_genericfx.h
 *
 *  @brief Base header for generic effect units
 *
 */

#ifndef UNIT_GENERICFX_H_
#define UNIT_GENERICFX_H_

#include <stdint.h>

#include "unit.h"
#include "osc_api.h"
#include "fx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
    * Obtain a reference to the current raw input buffer (before input wet/dry routing crossfade).
    *
    * The result is only valid when called from within a call to the render() callback, and must be
    * refreshed at every render() call.
    * This API was added to allow pre-buffering the audio input before the effect is enabled, such as needed
    * in looper-type effects.
    *
    * \return Reference to raw input buffer for current render() cycle.
    */
  typedef const float * (*unit_runtime_genericfx_get_raw_input_ptr)(void);
  
  typedef struct unit_runtime_genericfx_context {
    unit_runtime_base_context_fields
    uint32_t touch_area_width;
    uint32_t touch_area_height;
    unit_runtime_genericfx_get_raw_input_ptr get_raw_input;
  } unit_runtime_genericfx_context_t;
  
  enum {
    k_num_unit_genericfx_fixed_param_id = 0,
  };

#define UNIT_GENERICFX_MAX_PARAM_COUNT (UNIT_MAX_PARAM_COUNT)

  enum {
    k_genericfx_param_assign_none = 0U,
    k_genericfx_param_assign_x,
    k_genericfx_param_assign_y,
    k_genericfx_param_assign_depth,
    k_num_genericfx_param_assign
  };

  enum {
    k_genericfx_curve_linear = 0U,
    k_genericfx_curve_exp,
    k_genericfx_curve_log,
    k_genericfx_curve_toggle,
    k_genericfx_curve_minclip,
    k_genericfx_curve_maxclip,
    k_num_genericfx_curve
  };

  enum {
    k_genericfx_curve_unipolar = 0U,
    k_genericfx_curve_bipolar,
    k_num_genericfx_curve_polarity
  };
  
#pragma pack(push, 1)
  typedef struct genericfx_param_mapping {
    uint8_t assign;
    uint8_t curve:7;
    uint8_t curve_polarity:1;
    int16_t min;
    int16_t max;
    int16_t value;
  } genericfx_param_mapping_t;
#pragma pack(pop)

#pragma pack(push, 1)
  typedef struct genericfx_unit_header {
    unit_header_t common;
    genericfx_param_mapping_t default_mappings[UNIT_GENERICFX_MAX_PARAM_COUNT];
  } genericfx_unit_header_t;
#pragma pack(pop)

  extern const genericfx_unit_header_t unit_header;
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif // UNIT_GENERICFX_H_

/** @} */
