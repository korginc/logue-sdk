/*
 * File: biquad.cpp
 *
 * Test SDRAM memory i/o for delay lines
 *
 * 
 * 
 * 2018 (c) Korg
 *
 */

#include "usermodfx.h"

#include "delayline.hpp"

static dsp::DualDelayLine s_delay;

static __sdram f32pair_t s_delay_ram[8192];

static float s_len_z, s_len;
static const float s_fs_recip = 1.f / 48000.f;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
  s_delay.setMemory(s_delay_ram, 8192);  
  s_len = s_len_z = 1.f;
}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames)
{
  const float * mx = main_xn;
  float * __restrict my = main_yn;
  const float * my_e = my + 2*frames;

  const float *sx = sub_xn;
  float * __restrict sy = sub_yn;
  
  const float len = s_len;
  float len_z = s_len_z;
  
  for (; my != my_e; ) {
    
    *(my++) = *(mx++);
    *(sy++) = *(sx++);
    
    len_z = linintf(0.00004f, len_z, len);
    
    const f32pair_t r = s_delay.readFrac(len_z);
    s_delay.write((f32pair_t){*(mx++), *(sx++)});
    
    *(my++) = r.a;
    *(sy++) = r.b;
  }

  s_len_z = len_z;
}


void MODFX_PARAM(uint8_t index, int32_t value)
{
  const float valf = q31_to_f32(value);
  switch (index) {
  case k_user_modfx_param_time:
    break;
  case k_user_modfx_param_depth:
    s_len = 1 + valf * valf * 0.1f * 48000.f; // up to 100ms delay
    break;
  default:
    break;
  }
}

