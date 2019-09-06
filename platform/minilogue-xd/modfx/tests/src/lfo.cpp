/*
 * File: lfo.cpp
 *
 * Simple runtime test using LFO class as audio rate oscillator
 *
 * 
 * 
 * 2018 (c) Korg
 *
 */

#include "usermodfx.h"

#include "simplelfo.hpp"

static dsp::SimpleLFO s_lfo;

enum {
  k_sin = 0,
  k_tri,
  k_saw,
  k_sqr,
  k_sin_uni,
  k_tri_uni,
  k_saw_uni,
  k_sqr_uni,
  k_sin_off,
  k_tri_off,
  k_saw_off,
  k_sqr_off,
  k_wave_count
};

static uint8_t s_lfo_wave;
static float s_param_z, s_param;
static const float s_fs_recip = 1.f / 48000.f;

void MODFX_INIT(uint32_t platform, uint32_t api)
{
  s_lfo.reset();
  s_lfo.setF0(220.f,s_fs_recip);
}

void MODFX_PROCESS(const float *main_xn, float *main_yn,
                   const float *sub_xn,  float *sub_yn,
                   uint32_t frames)
{
  float * __restrict my = main_yn;
  const float * my_e = my + 2*frames;
  float * __restrict sy = sub_yn;

  const float p = s_param;
  float p_z = s_param_z;
  
  for (; my != my_e; ) {

    p_z = linintf(0.002f, p_z, p);
    
    s_lfo.cycle();

    float wave;
    
    switch (s_lfo_wave) {
    case k_sin:
      wave = s_lfo.sine_bi();
      break;
      
    case k_tri:
      wave = s_lfo.triangle_bi();
      break;
      
    case k_saw:
      wave = s_lfo.saw_bi();
      break;
      
    case k_sqr:
      wave = s_lfo.square_bi();
      break;
      
    case k_sin_uni:
      wave = s_lfo.sine_uni();
      break;

    case k_tri_uni:
      wave = s_lfo.triangle_uni();
      break;

    case k_saw_uni:
      wave = s_lfo.saw_uni();
      break;

    case k_sqr_uni:
      wave = s_lfo.square_uni();
      break;

    case k_sin_off:
      wave = s_lfo.sine_bi_off(s_param_z);
      break;

    case k_tri_off:
      wave = s_lfo.triangle_bi_off(s_param_z);
      break;

    case k_saw_off:
      wave = s_lfo.saw_bi_off(s_param_z);
      break;

    case k_sqr_off:
      wave = s_lfo.square_bi_off(s_param_z);
      break;
    }
    
    // Scale down the wave, full swing is way too loud. (polyphony headroom)
    wave *= 0.1f;
    
    *(my++) = wave;
    *(my++) = wave;
    *(sy++) = wave;
    *(sy++) = wave;
  }

  s_param_z = p_z;
}


void MODFX_PARAM(uint8_t index, int32_t value)
{
  const float valf = q31_to_f32(value);
  switch (index) {
  case k_user_modfx_param_time:
    s_lfo_wave = si_roundf(valf * (k_wave_count - 1));
    break;
  case k_user_modfx_param_depth:
    s_param = valf;
    break;
  default:
    break;
  }
}

