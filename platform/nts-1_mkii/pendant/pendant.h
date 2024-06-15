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
 *  File: pendant.h
 *
 *  Phase distortion oscillator inspired by Casio CZ synthesizers
 *  paper link: http://recherche.ircam.fr/pub/dafx11/Papers/55_e.pdf
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>

#include "unit_osc.h"   // Note: Include base definitions for osc units

#include "utils/int_math.h"   // for clipminmaxi32()

// #include "wavetables.h"

float my_wt(float w, float pos)
{
  pos *= 2.9999f;
  const int lpf=1; // higher value uses more bandlimited wavetable
  if (pos <=1.f)
  {
    pos = modff(pos, nullptr);
    return linintf(pos, osc_sinf(w), osc_bl_parf(w, lpf));
  } 
  else if (pos <= 2.f)
  {
    pos = modff(pos, nullptr);
    return linintf(pos, osc_bl_parf(w, lpf), osc_bl_sqrf(w, lpf));
  }
  else
  {
    pos = modff(pos, nullptr);
    return linintf(pos, osc_bl_sqrf(w, lpf), osc_bl_sawf(w, lpf));
  }
}

class Pendant {
 public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  enum {
    SHAPE = 0U,
    ALT,
    WAVE,
    NUM_PARAMS
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params {
    float shape{0.f};
    float alt{0.f};
    float wave{0.f};

    void reset() {
      shape = 0.f;
      alt = 0.f;
      wave = 0.f;
    }
  };

  // enum {
  //   SINE = 0,
  //   TRI,
  //   SQAURE,
  //   SAW,
  //   NUM_WAVE_VALUES,
  // };

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Pendant(void) {}
  ~Pendant(void) {} // Note: will never actually be called for statically allocated instances

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

  // input: original phase in the range of [0,1)
  // output: distorted phase, which might be greater than 1
  float phaseshaper(float w, float d, int v)
  {
    if (w < d) {
      w = v/d *w;
    } else {
      w = v + (1.f-v)/(1.f-d) *(w-d);
    }

    // aliaing suppresion
    // if (v > 1 && w > floorf(v)) {
    //   float b = modff(v, nullptr);
    //   if (b <=0.5) {
    //     w = modff(w,nullptr)/(2*b);
    //   }
    //   else {
    //     w = modff(w,nullptr)/b;
    //   }
    // } 
    return w;
  }



  fast_inline void Process(const float * in, float * out, size_t frames) {
    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + frames;  // assuming mono output

    // Caching current parameter values. Consider interpolating sensitive parameters.
    const Params p = params_;

    // get osc pitch from context
    const unit_runtime_osc_context_t *ctxt = static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);
    float w0 = osc_w0f_for_note((ctxt->pitch)>>8, ctxt->pitch & 0xFF);
    float lfo = q31_to_f32(ctxt->shape_lfo);

    for (; out_p != out_e; in_p += 2, out_p += 1) {
      // Process/generate samples here

      

      float deflection_point_target = linintf(clip01f(p.shape+lfo), 0.5f, 0.1f);
      // float deflection_point_target = linintf(p.shape, 0.5f, 0.1f);
      // deflection_point_target = clipminmaxf(0.01f, deflection_point_target+lfo, 0.99f);
      deflection_point = linintf(0.03f, deflection_point, deflection_point_target); //smoothing
      float value_target = linintf(p.alt, 0.5f, 10.f);
      value = linintf(0.03f, value, value_target); //smoothing
      float position_target = linintf(p.wave, 0.f, 1.f);
      position = linintf(0.03f, position, position_target); //smoothing


      // if (value<=1.f)
      // {
      //   float w_distorted = phaseshaper(w, deflection_point, value);
      //   *out_p = my_wt(w_distorted, position);
      // }
      // else
      {
        float value_low;
        float fraction_part = modff(value, &value_low);
        float value_high = value_low+1;
        float w_distorted_1 = phaseshaper(w, deflection_point, (int)value_low);
        float osc1 = my_wt(w_distorted_1, position);
        float w_distorted_2 = phaseshaper(w, deflection_point, (int)value_high);
        float osc2 = my_wt(w_distorted_2, position);
        *out_p = linintf(fraction_part, osc1, osc2);
      }

      // late phasor update
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

    case WAVE:
      value = clipminmaxi32(0, value, 1023);
      params_.wave = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
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

    case WAVE:
      return param_f32_to_10bit(params_.wave);
      break;

    default:
      break;
    }

    return INT_MIN; // Note: will be handled as invalid
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue
    
    // static const char * wave_strings[NUM_WAVE_VALUES] = {
    //   "SIN",
    //   "TRI",
    //   "SQR",
    //   "SAW",
    // };
    
    switch (index) {
    // case WAVE:
    //   if (value >= 0 && value < NUM_WAVE_VALUES)
    //     return wave_strings[value];
    //   break;
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
    w = 0.f; // note: phase reset does not make initial clicking disappear
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

  float w{0.f}; // phasor in [0,1]
  float deflection_point{0.5f};
  float value{0.5f};
  float position{0.f};

  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
};
