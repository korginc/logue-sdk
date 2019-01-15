/*
 * File: delayline.cpp
 *
 * Test SDK Bi-Quad
 *
 * 
 * 
 * 2018 (c) Korg
 *
 */

#include "userrevfx.h"

#include "delayline.hpp"

static dsp::DelayLine s_delay;

static __sdram float s_delay_ram[8192];

static float s_len_z, s_len;
static const float s_fs_recip = 1.f / 48000.f;

void REVFX_INIT(uint32_t platform, uint32_t api)
{
  s_delay.setMemory(s_delay_ram, 8192);  
  s_len = s_len_z = 1;
}

void REVFX_PROCESS(float *xn, uint32_t frames)
{
  float * __restrict x = xn;
  const float * x_e = x + 2*frames;
  
  const float len = s_len;
  float len_z = s_len_z;
  
  for (; x != x_e; ) {
    
    *(x++);
    
    len_z = linintf(0.00004f, len_z, len);
    
    const float r = 0.25f * s_delay.readFrac(len_z);
    s_delay.write(*x);
    *(x++) += r;
  }

  s_len_z = len_z;
}


void REVFX_PARAM(uint8_t index, int32_t value)
{
  const float valf = q31_to_f32(value);
  switch (index) {
  case 0:
    break;
  case 1:
    s_len = 1 + valf * valf * 0.1f * 48000.f; // up to 100ms delay
    break;
  default:
    break;
  }
}

