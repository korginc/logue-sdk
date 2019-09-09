/*
 * File: delayline.cpp
 *
 * Test SDRAM memory i/o for delay lines
 *
 * 
 * 
 * 2018 (c) Korg
 *
 */

#include "userrevfx.h"

#include "delayline.hpp"

static dsp::DelayLine s_delay;

static __sdram float s_delay_ram[48000];

static float s_len_z, s_len;
static float s_mix;
static const float s_fs_recip = 1.f / 48000.f;

void REVFX_INIT(uint32_t platform, uint32_t api)
{
  s_delay.setMemory(s_delay_ram, 48000);  
  s_len = s_len_z = 1.f;
  s_mix = 1.f;
}

void REVFX_PROCESS(float *xn, uint32_t frames)
{
  float * __restrict x = xn;
  const float * x_e = x + 2*frames;
  
  const float len = s_len;
  float len_z = s_len_z;

  const float dry = 1.f - s_mix;
  const float wet = s_mix;
  
  for (; x != x_e; ) {
    
    *(x++);
    
    len_z = linintf(0.00004f, len_z, len);
    
    const float r = 0.25f * s_delay.readFrac(len_z);
    s_delay.write(*x);
    *(x++) = dry * (*x) + wet * r;
  }

  s_len_z = len_z;
}


void REVFX_PARAM(uint8_t index, int32_t value)
{
  const float valf = q31_to_f32(value);
  switch (index) {
  case k_user_revfx_param_time:
    break;
  case k_user_revfx_param_depth:
    s_len = 1 + valf * valf * 0.1f * 48000.f; // up to 100ms delay
    break;
  case k_user_revfx_param_shift_depth:
    // Rescale to add notch around 0.5f
    s_mix = (valf <= 0.49f) ? 1.02040816326530612244f * valf : (valf >= 0.51f) ? 0.5f + 1.02f * (valf-0.51f) : 0.5f;
    break;
  default:
    break;
  }
}

