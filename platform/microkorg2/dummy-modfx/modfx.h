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
 *  File: modfx.h
 *
 *  Dummy modfx effect template instance.
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <iostream>

#include "utils/buffer_ops.h" // for buf_clr_f32()
#include "utils/int_math.h"   // for clipminmaxi32()
#ifndef __EMSCRIPTEN__
#include "utils/mk2_utils.h"
#endif
#include "runtime.h"
#include "unit_modfx.h"
#include "macros.h"
#include "processor.h"

class ModFx : public Processor
{
public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  uint32_t getBufferSize() const override final { return MODFX_MEMORY_SIZE; }

  enum
  {
    TIME = 0U,
    DEPTH,
    PARAM3,
    NUM_PARAMS
  };

  enum
  {
    PARAM3_VALUE0 = 0,
    PARAM3_VALUE1,
    PARAM3_VALUE2,
    PARAM3_VALUE3,
    NUM_PARAM3_VALUES,
  };

  // Note: Make sure default param values correspond to declarations in header.c
  struct Params
  {
    float time;
    float depth;
    uint32_t param3;

    void reset()
    {
      time = 0.25f;
      depth = 0.25f;
      param3 = 1;
    }

    Params() { reset(); }
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case TIME:
    {
      // 10bit 0-1023 parameter
      params_.time = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      break;
    }
    case DEPTH:
    {
      // 10bit 0-1023 parameter
      params_.depth = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      break;
    }
    case PARAM3:
      params_.param3 = value;
      break;

    default:
      break;
    }
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue

    static const char *param3_strings[NUM_PARAM3_VALUES] = {
        "VAL 0",
        "VAL 1",
        "VAL 2",
        "VAL 3",
    };

    switch (index)
    {
    case PARAM3:
      if (value >= PARAM3_VALUE0 && value < NUM_PARAM3_VALUES)
        return param3_strings[value];
      break;
    default:
      break;
    }

    return nullptr;
  }

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  void init(float *allocated_buffer) override final
  {
    buffer_ = allocated_buffer;
    params_.reset();
  }

  void teardown() override final { buffer_ = nullptr; }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void process(const float *__restrict in, float *__restrict out, uint32_t frames, uint32_t outChannels) override final
  {
    // Caching current parameter values. Consider smoothing sensitive parameters in audio loop
    const Params p = params_;

    for (const float *out_end = out + frames * outChannels; out != out_end; in += 2, out += outChannels)
    {
      // Process samples here

      // pass through
      out[0] = in[0]; // left sample
      out[1] = in[1]; // right sample
    }
  }

  /*===========================================================================*/
  /* Static Members. */
  /*===========================================================================*/

private:
  float *buffer_;
  Params params_;
};
