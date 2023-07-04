/*
    BSD 3-Clause License

    Copyright (c) 2018, KORG INC.
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
 * File: sine.cpp
 *
 * Naive sine oscillator test
 *
 */

#include "userosc.h"
typedef __uint32_t uint32_t; // VSCode understanding of __uint32_t

typedef struct State
{
  float w0;
  float phase;
  float percAmount; // positive nad negative for harm
  float percEnv;    // trending towards zero, make usre to turn off zero math
  float harmxlvl[7];
  uint8_t harmxNum[7];
  uint8_t flags;
} State;

static State s_state;

enum
{
  // you have 8 flags available, name them better
  k_flags_none = 0,
  k_flag_reset = 1,
};

void OSC_INIT(uint32_t platform, uint32_t api)
{
  s_state.w0 = 0.f;
  s_state.phase = 0.f;
  s_state.flags = k_flags_none;
  s_state.harmxNum[0] = 0;
  s_state.harmxNum[1] = 1;
  s_state.harmxNum[2] = 2;
  s_state.harmxNum[3] = 3;
  s_state.harmxNum[4] = 4;
  s_state.harmxNum[5] = 6;
  s_state.harmxNum[6] = 8;
  for (int i = 0; i < 7; i++)
  {
    s_state.harmxlvl[i] = i < 4 ? 1.f : 0.f;
  }
}

void OSC_CYCLE(const user_osc_param_t *const params,
               int32_t *yn,
               const uint32_t frames)
{
  const uint8_t flags = s_state.flags;
  s_state.flags = k_flags_none;

  const float w0 = s_state.w0 = osc_w0f_for_note((params->pitch) >> 8, params->pitch & 0xFF);
  const float freq = osc_notehzf((params->pitch) >> 8);
  float phase = s_state.phase;
  const float fbFreq = 5930.f; // actual highest note is 5924.62Hz

  q31_t *__restrict y = (q31_t *)yn;
  const q31_t *y_e = y + frames;

  for (; y != y_e; y++)
  {
    float accumulator = 0.f;
    float foldbackRatio = 1.f;
    for (int i = 1; i < 7; i++)
    {
      float harmx = s_state.harmxNum[i];
      float harmxLvl = s_state.harmxlvl[i];
      if (freq * harmx * foldbackRatio > fbFreq)
      {
        foldbackRatio *= 0.5f;
      }
      accumulator += osc_sinf(phase * harmx * foldbackRatio) * harmxLvl;
    }

    *(y) = f32_to_q31(accumulator * .14f);

    phase += w0; // advance phase

    if (phase > 8.f)
    {               // wraps phase, larger to accomodate foldback.
      phase -= 8.f; // larger numbers have larger phase error, but prevent foldback bugs
    }
  }

  s_state.phase = phase;
}

void OSC_NOTEON(const user_osc_param_t *const params)
{
  // perc effects
  (void)params;
}

void OSC_NOTEOFF(const user_osc_param_t *const params)
{
  // leave blank
  (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  const float valf = param_val_to_f32(value);

  switch (index)
  {
  case k_user_osc_param_id1:
    s_state.harmxlvl[1] = valf;
    break;
  case k_user_osc_param_id2:
    s_state.harmxlvl[2] = valf;
    break;
  case k_user_osc_param_id3:
    s_state.harmxlvl[3] = valf;
    break;
  case k_user_osc_param_id4:
    s_state.harmxlvl[4] = valf;
    break;
  case k_user_osc_param_id5:
    s_state.harmxlvl[5] = valf;
    break;
  case k_user_osc_param_id6:
    s_state.harmxlvl[6] = valf;
    break;
    break;
  case k_user_osc_param_shape:
    break;
  case k_user_osc_param_shiftshape:
    break;
  default:
    break;
  }
}