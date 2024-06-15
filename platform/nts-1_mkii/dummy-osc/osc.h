#pragma once
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
 *  File: osc.h
 *
 *  Dummy oscillator template instance.
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>

#include "unit_osc.h"   // Note: Include base definitions for osc units

#include "utils/int_math.h"   // for clipminmaxi32()

class Osc {
 public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  enum {
    SHAPE = 0U,
    ALT,
    PARAM3, 
    NUM_PARAMS
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params {
    float shape{0.f};
    float alt{0.f};
    uint32_t param3{1};

    void reset() {
      shape = 0.f;
      alt = 0.f;
      param3 = 1;
    }
  };

  enum {
    PARAM3_VALUE0 = 0,
    PARAM3_VALUE1,
    PARAM3_VALUE2,
    PARAM3_VALUE3,
    NUM_PARAM3_VALUES,
  };

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Osc(void) {}
  ~Osc(void) {} // Note: will never actually be called for statically allocated instances

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (!desc)
      return k_unit_err_undef;
    
    // Note: make sure the unit is being loaded to the correct platform/module target
    if (desc->target != unit_header.target)
      return k_unit_err_target;
    
    // Note: check API compatibility with the one this unit was built against
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;

    // Check compatibility of samplerate with unit, for NTS-1 MKII should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    // Note: NTS-1 mkII oscillators can make use of the audio input depending on the routing options in global settings, see product documentation for details.
    if (desc->input_channels != 2 || desc->output_channels != 1)  // should be stereo input / mono output
      return k_unit_err_geometry;

    // Note: SDRAM is not available from the oscillator runtime environment
    
    // Cache the runtime descriptor for later use
    runtime_desc_ = *desc;

    // Make sure parameters are reset to default values
    params_.reset();
    
    return k_unit_err_none;
  }

  inline void Teardown() {
    // Note: cleanup and release resources if any
  }

  inline void Reset() {
    // Note: Reset effect state, excluding exposed parameter values.
  }

  inline void Resume() {
    // Note: Effect will resume and exit suspend state. Usually means the synth
    // was selected and the render callback will be called again
  }

  inline void Suspend() {
    // Note: Effect will enter suspend state. Usually means another effect was
    // selected and thus the render callback will not be called
  }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void Process(const float * in, float * out, size_t frames) {
    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + frames;  // assuming mono output

    // Caching current parameter values. Consider interpolating sensitive parameters.
    // const Params p = params_;

    // get osc pitch from context
    const unit_runtime_osc_context_t *ctxt = static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);
    float w0 = osc_w0f_for_note((ctxt->pitch)>>8, ctxt->pitch & 0xFF);
    float lfo = q31_to_f32(ctxt->shape_lfo); // TODO: Apply shape_lfo to the shape parameter
    
    for (; out_p != out_e; in_p += 2, out_p += 1) {
      // Process/generate samples here
      
      // Note: this is a dummy unit only to demonstrate APIs, only outputting sin wave.
      *out_p = osc_sinf(w);

      // update phase here
      w += w0;
      w = modff(w, nullptr); // take fractional part to prevent overflow

    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    switch (index) {
    case SHAPE:
      // 10bit 0-1023 parameter
      value = clipminmaxi32(0, value, 1023);
      params_.shape = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      break;

    case ALT:
      // 10bit 0-1023 parameter
      value = clipminmaxi32(0, value, 1023);
      params_.alt = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      break;

    case PARAM3:
      // strings type parameter, receiving index value
      value = clipminmaxi32(PARAM3_VALUE0, value, NUM_PARAM3_VALUES-1);
      params_.param3 = value;
      break;
      
    default:
      break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
    case SHAPE:
      // 10bit 0-1023 parameter
      return param_f32_to_10bit(params_.shape);
      break;

    case ALT:
      // 10bit 0-1023 parameter
      return param_f32_to_10bit(params_.alt);
      break;

    case PARAM3:
      // strings type parameter, return index value
      return params_.param3;

    default:
      break;
    }

    return INT_MIN; // Note: will be handled as invalid
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue
    
    static const char * param3_strings[NUM_PARAM3_VALUES] = {
      "VAL 0",
      "VAL 1",
      "VAL 2",
      "VAL 3",
    };
    
    switch (index) {
    case PARAM3:
      if (value >= PARAM3_VALUE0 && value < NUM_PARAM3_VALUES)
        return param3_strings[value];
      break;
    default:
      break;
    }
    
    return nullptr;
  }

  inline void setTempo(uint32_t tempo) {
    // const float bpmf = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
    (void)tempo;
  }

  inline void tempo4ppqnTick(uint32_t counter) {
    (void)counter;
  }

  inline void NoteOn(uint8_t note, uint8_t velo) {
    (uint8_t)note;
    (uint8_t)velo;
  }

  inline void NoteOff(uint8_t note) {
    (uint8_t)note;
  }

  inline void AllNoteOff() {
    // resetting the phase to zero for a more consistent attack
    w = 0.f;
  }

  inline void PitchBend(uint8_t bend) {
    (uint8_t)bend;
  }

  inline void ChannelPressure(uint8_t press) {
    (uint8_t)press;
  }

  inline void AfterTouch(uint8_t note, uint8_t press) {
    (uint8_t)note;
    (uint8_t)press;
  }

  
  /*===========================================================================*/
  /* Static Members. */
  /*===========================================================================*/
  
 private:
  /*===========================================================================*/
  /* Private Member Variables. */
  /*===========================================================================*/

  std::atomic_uint_fast32_t flags_;

  unit_runtime_desc_t runtime_desc_;

  Params params_;

  float w{0.f}; // phasor in [0.0,1.0)

  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
};
