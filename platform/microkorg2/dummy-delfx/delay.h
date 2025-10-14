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
 *  File: delay.h
 *
 *  Dummy delay effect template instance.
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <iostream>

#include "utils/buffer_ops.h" // for buf_clr_f32()
#include "utils/int_math.h"   // for clipminmaxi32()
#include "utils/mk2_utils.h"
#include "runtime.h"
#include "unit_delfx.h"
#include "macros.h"

class Delay {
 public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  enum {
    BUFFER_LENGTH = DELFX_MEMORY_SIZE,
  };

  enum {
    kDelayTime = 0U,
    kDepth,
    kMix, 
    kParam4,
    kNumParams
  };

  enum {
    PARAM4_VALUE0 = 0,
    PARAM4_VALUE1,
    PARAM4_VALUE2,
    PARAM4_VALUE3,
    NUM_PARAM4_VALUES,
  };
  
  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Delay(void):
  mTempo(120),
  mTime(0),
  mMix(0),
  mDepth(0),
  allocated_buffer_(nullptr)
  {

  }

  ~Delay(void) 
  {} // Note: will never actually be called for statically allocated instances

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (!desc)
      return k_unit_err_undef;
    
    // Note: make sure the unit is being loaded to the correct platform/module target
    if (desc->target != unit_header.target)
      return k_unit_err_target;
    
    // Note: check API compatibility with the one this unit was built against
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;
    
    // Check compatibility of samplerate with unit, for microkorg2 should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    if (desc->input_channels != 2 || desc->output_channels != 2)  // should be stereo input/output
      return k_unit_err_geometry;

    // If SDRAM buffers are required they must be allocated here
    if (!desc->hooks.sdram_alloc)
      return k_unit_err_memory;
    float *m = (float *)desc->hooks.sdram_alloc(BUFFER_LENGTH * sizeof(float));
    if (!m)
      return k_unit_err_memory;

    // microkorg2 handles buffer clearing
    // buf_clr_f32(m, BUFFER_LENGTH);

    allocated_buffer_ = m;
    
    // Cache the runtime descriptor for later use
    runtime_desc_ = *desc;

    buf_clr_u32(reinterpret_cast<uint32_t *>(params_), kNumParams);
    
    return k_unit_err_none;
  }

  inline void Teardown() {
    // Note: buffers allocated via sdram_alloc are automatically freed after unit teardown
    // Note: cleanup and release resources if any
    allocated_buffer_ = nullptr;
  }

  inline void Reset() {
    // Note: Reset effect state, excluding exposed parameter values.
  }

  inline void Resume() {
    // Note: Effect will resume and exit suspend state. Usually means the synth
    // was selected and the render callback will be called again

    // Note: If it is required to clear large memory buffers, consider setting a flag
    //       and trigger an asynchronous progressive clear on the audio thread (Process() handler)
  }

  inline void Suspend() {
    // Note: Effect will enter suspend state. Usually means another effect was
    // selected and thus the render callback will not be called
  }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void Process(const float * in, float * out, size_t frames) 
  {
    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1);  // assuming stereo output

    for (; out_p != out_e; in_p += 2, out_p += 2) {
      // Process samples here
      
      // Note: this is a dummy unit only to demonstrate APIs, only passing through audio
      out_p[0] = in_p[0]; // left sample
      out_p[1] = in_p[1]; // right sample
    }
  }

  inline void setParameter(uint8_t index, int32_t value) 
  {
    params_[index] = value;
    switch (index)
    {
      case kDelayTime:
      {
        // 10bit 0-1023 parameter
        mTime = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
        break;
      }
      case kDepth:
      {
        // 10bit 0-1023 parameter
        mDepth = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
        break;
      }
      case kMix:
      {
        // Single digit base-10 fractional value, bipolar dry/wet
        mMix = value / 1000.f; // -100.0 .. 100.0 -> -1.0 .. 1.0
        break;
      }
      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const
  {
    return params_[index];
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue
    
    static const char * param4_strings[NUM_PARAM4_VALUES] = {
      "VAL 0",
      "VAL 1",
      "VAL 2",
      "VAL 3",
    };
    
    switch (index) {
    case kParam4:
      if (value >= PARAM4_VALUE0 && value < NUM_PARAM4_VALUES)
        return param4_strings[value];
      break;
    default:
      break;
    }
    
    return nullptr;
  }

  inline void setTempo(uint32_t tempo) {
    mTempo = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
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

  float mTempo;

  int32_t params_[kNumParams];
  float mTime;
  float mMix;
  float mDepth;

  float * allocated_buffer_;
  
  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
};
