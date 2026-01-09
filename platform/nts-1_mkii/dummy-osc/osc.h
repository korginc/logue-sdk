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
#include "processor.h"
#include "unit_osc.h"

class Osc : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; } // NTS-1 osc do not support sdram allocation

  // audio parameters
  enum
  {
    SHAPE = 0U,
    ALT,
    PARAM3,
    NUM_PARAMS
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params
  {
    float shape;
    float alt;
    uint32_t param3;

    void reset()
    {
      shape = 0.f;
      alt = 0.f;
      param3 = 1;
    }

    Params() { reset(); }
  };

  enum
  {
    PARAM3_VALUE0 = 0,
    PARAM3_VALUE1,
    PARAM3_VALUE2,
    PARAM3_VALUE3,
    NUM_PARAM3_VALUES,
  };

  void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case SHAPE:
      params_.shape = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      break;

    case ALT:
      params_.alt = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      break;

    case PARAM3:
      params_.param3 = value; // string type, receiving index
      break;

    default:
      break;
    }
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final
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

  // life-cycle methods
  void init(float *) override final
  {
    params_.reset();
    phasor_ = 0.f;
  }

  // audio processing callbacks

  // set frequency in digital w (w = f/samplerate, 0.5 is Nyquist)
  void setPitch(float w0)
  {
    w0_ = w0; // use this as the phase increment for oscillator
  }

  // lfo in (-1.0f, 1.0f)
  void setShapeLfo(float lfo)
  {
    lfo_ = lfo;
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    // Caching current parameter values. Consider smoothing sensitive parameters in audio loop
    const Params p = params_;

    for (const float *out_end = out + frames; out != out_end; in += 2, out += 1)
    {
      // Process/generate samples here

      // phasor update
      phasor_ = fmodf(phasor_ + w0_, 1.f);

      // read sine wave table
      out[0] = osc_sinf(phasor_);
    }
  }

private:
  Params params_;
  float w0_;
  float lfo_;

  // local variables related to audio processing
  float phasor_;
};
