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
 *  @file unit_osc.h
 *
 *  @brief Base header for oscillator units
 *
 */

#ifndef UNIT_OSC_H_
#define UNIT_OSC_H_

#include <stdint.h>

#include "unit.h"
#include "osc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Oscillator input usage states
   */
  enum {
    k_runtime_osc_input_unused = 0U,
    k_runtime_osc_input_used,
  };

  /**
   * Pointer to notify_input_usage(uint8_t usage), used to notify the runtime that oscillator is using the audio input or not.
   * Note: the runtime assumes audio input is unused by default
   */
  typedef void (*unit_runtime_osc_notify_input_usage_ptr)(uint8_t);
  
  /** Oscillator specific unit runtime context. */
  typedef struct unit_runtime_osc_context {
    int32_t  shape_lfo;      // Shape LFO signal encoded in Q31 fixed point format
    uint16_t pitch;          // Upper 8 bits: note, Lower 8 bits: inter-note fraction
    uint16_t cutoff;         // Unused. Future.
    uint16_t resonance;      // Unused. Future.
    uint8_t  amp_eg_phase;   // Unused. Future.
    uint8_t  amp_eg_state:3; // Unused. Future.
    uint8_t  padding0:5;
    unit_runtime_osc_notify_input_usage_ptr notify_input_usage;
  } unit_runtime_osc_context_t;

  /** Exposed parameters with fixed/direct UI controls. */
  enum {
    k_unit_osc_fixed_param_shape = 0,
    k_unit_osc_fixed_param_altshape,
    k_num_unit_osc_fixed_param_id
  };  

#define UNIT_OSC_MAX_PARAM_COUNT (10)
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif // UNIT_OSC_H_
