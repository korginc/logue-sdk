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
#include "runtime.h"
#include "utils/float_simd.h"
#include "utils/int_simd.h"
#include "utils/int_math.h"
#include "utils/float_math.h"
#include "utils/mk2_osc_api.h"

#ifdef __cplusplus
extern "C" {
#endif
  
  #define OSC_MEMORY_SIZE_BYTES 0x2000
  #define OSC_MEMORY_SIZE (OSC_MEMORY_SIZE_BYTES >> 2) // assumes 32 bit data type
  #define UNIT_OSC_MAX_PARAM_COUNT (13)

  /** Oscillator specific unit runtime context. */
  typedef struct unit_runtime_osc_context 
  {
    float pitch[kMk2MaxVoices]; 
    uint8_t trigger; // bit array
    float * unitModDataPlus; 
    float * unitModDataPlusMinus; 
    uint8_t modDataSize;
    uint16_t bufferOffset;
    uint8_t voiceOffset;
    uint8_t voiceLimit;
    uint16_t outputStride;
  } unit_runtime_osc_context_t;

  uint32_t GetBufferOffset(const unit_runtime_osc_context_t * context, int voice, int32_t frameSize) 
  { 
    return context->bufferOffset + (voice >> 2) * (frameSize << 2); 
  }
  
  // -------------------------------------
  // ------- mod source helpers ----------
  // -------------------------------------
  void WriteUnitModDataPlusx1(const unit_runtime_osc_context_t * context, float mod, uint8_t voice)
  {
    voice = (voice >= context->modDataSize) ? context->modDataSize - 1 : voice;
    context->unitModDataPlus[voice] = clip01f(mod);
  }

  void WriteUnitModDataPlusMinusx1(const unit_runtime_osc_context_t * context, float mod, uint8_t voice)
  {
    voice = (voice >= context->modDataSize) ? context->modDataSize - 1 : voice;
    context->unitModDataPlusMinus[voice] = clip1m1f(mod);
  }

  // assumes input is within -1 ~ 1
  void WriteUnitModDatax1(const unit_runtime_osc_context_t * context, float mod, uint8_t voice)
  {
    voice = (voice >= context->modDataSize) ? context->modDataSize - 1 : voice;
    context->unitModDataPlusMinus[voice] = clip1m1f(mod);
    context->unitModDataPlus[voice] = context->unitModDataPlusMinus[voice] * 0.5f + 0.5f;
  }

  void WriteUnitModDataPlusx2(const unit_runtime_osc_context_t * context, float32x2_t mod, uint8_t startVoice)
  {
    startVoice = clipminmaxi32(0, startVoice, context->modDataSize - 1);
    f32x2_str(&context->unitModDataPlus[startVoice], clip01fx2(mod));
  }

  void WriteUnitModDataPlusMinusx2(const unit_runtime_osc_context_t * context, float32x2_t mod, uint8_t startVoice)
  {
    startVoice = clipminmaxi32(0, startVoice, context->modDataSize - 1);
    f32x2_str(&context->unitModDataPlusMinus[startVoice], clip1m1fx2(mod));
  }

  // assumes input is within -1 ~ 1
  void WriteUnitModDatax2(const unit_runtime_osc_context_t * context, float32x2_t mod, uint8_t startVoice)
  {
    startVoice = clipminmaxi32(0, startVoice, context->modDataSize - 1);
    float32x2_t clipped = clip01fx2(mod);
    f32x2_str(&context->unitModDataPlusMinus[startVoice], clipped);
    f32x2_str(&context->unitModDataPlus[startVoice], float32x2_addscal(float32x2_mulscal(clipped, 0.5f), 0.5f));
  }

  void WriteUnitModDataPlusx4(const unit_runtime_osc_context_t * context, float32x2_t mod, uint8_t startVoice)
  {
    startVoice = clipminmaxi32(0, startVoice, context->modDataSize - 1);
    f32x2_str(&context->unitModDataPlus[startVoice], clip01fx2(mod));
  }

  void WriteUnitModDataPlusMinusx4(const unit_runtime_osc_context_t * context, float32x4_t mod, uint8_t startVoice)
  {
    startVoice = clipminmaxi32(0, startVoice, context->modDataSize - 1);
    f32x4_str(&context->unitModDataPlusMinus[startVoice], clip1m1fx4(mod));
  }

  // assumes input is within -1 ~ 1
  void WriteUnitModDatax4(const unit_runtime_osc_context_t * context, float32x4_t mod, uint8_t startVoice)
  {
    startVoice = clipminmaxi32(0, startVoice, context->modDataSize - 1);
    float32x4_t clipped = clip01fx4(mod);
    f32x4_str(&context->unitModDataPlusMinus[startVoice], clipped);
    f32x4_str(&context->unitModDataPlus[startVoice], float32x4_addscal(float32x4_mulscal(clipped, 0.5f), 0.5f));
  }
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif // UNIT_OSC_H_
