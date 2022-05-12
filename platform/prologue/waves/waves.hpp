#pragma once
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
 *  File: waves.hpp
 *
 *  Morphing Wavetable Synthesizer
 *
 */

#include "userosc.h"
#include "biquad.hpp"

struct Waves {

  enum {
    k_flags_none    = 0,
    k_flag_wave0    = 1<<1,
    k_flag_wave1    = 1<<2,
    k_flag_subwave  = 1<<3,
    k_flag_ringmix  = 1<<4,
    k_flag_bitcrush = 1<<5,
    k_flag_reset    = 1<<6
  };
  
  struct Params {
    float    submix;
    float    ringmix;
    float    bitcrush;
    float    shape;
    float    shiftshape;
    uint8_t  wave0;
    uint8_t  wave1;
    uint8_t  subwave;
    uint8_t  padding;
    
    Params(void) :
      submix(0.05f),
      ringmix(0.f),
      bitcrush(0.f),
      shape(0.f),
      shiftshape(0.f),
      wave0(0),
      wave1(0),
      subwave(0)
    { }
  };
  
  struct State {
    const float   *wave0;
    const float   *wave1;
    const float   *subwave;
          float    phi0;
          float    phi1;
          float    phisub;
          float    w00;
          float    w01;
          float    w0sub;
          float    lfo;
          float    lfoz;
          float    dither;
          float    bitres;
          float    bitresrcp;
          float    imperfection;
          uint32_t flags:8;
    
    State(void) :
      wave0(wavesA[0]),
      wave1(wavesD[0]),
      subwave(wavesA[0]),
      w00(440.f * k_samplerate_recipf),
      w01(440.f * k_samplerate_recipf),
      w0sub(220.f * k_samplerate_recipf),
      lfo(0.f),
      lfoz(0.f),
      dither(0.f),
      bitres(1.f),
      bitresrcp(1.f),
      flags(k_flags_none)
    {
      reset();
      imperfection = osc_white() * 1.0417e-006f; // +/- 0.05Hz@48KHz
    }
    
    inline void reset(void)
    {
      phi0 = 0;
      phi1 = 0;
      phisub = 0;
      lfo = lfoz;
    }
  };

  Waves(void) {
    init();
  }

  void init(void) {
    state = State();
    params = Params();
    prelpf.mCoeffs.setPoleLP(0.8f);
    postlpf.mCoeffs.setFOLP(osc_tanpif(0.45f));
  }
  
  inline void updatePitch(float w0) {
    w0 += state.imperfection;
    const float drift = params.shiftshape;
    state.w00 = w0;
    // Alt osc with slight drift (0.25Hz@48KHz)
    state.w01 = w0 + drift * 5.20833333333333e-006f;
    // Sub one octave and a phase drift (0.15Hz@48KHz)
    state.w0sub = 0.5f * w0 + drift * 3.125e-006f;
  }
    
  inline void updateWaves(const uint16_t flags) {
    if (flags & k_flag_wave0) {
      static const uint8_t k_a_thr = k_waves_a_cnt;
      static const uint8_t k_b_thr = k_a_thr + k_waves_b_cnt;
      static const uint8_t k_c_thr = k_b_thr + k_waves_c_cnt;
      
      uint8_t idx = params.wave0;
      const float * const * table;
      
      if (idx < k_a_thr) {
        table = wavesA;
      }
      else if (idx < k_b_thr) {
        table = wavesB;
        idx -= k_a_thr;
      }
      else { 
        table = wavesC;
        idx -= k_b_thr;
      }
      state.wave0 = table[idx];
    }
    if (flags & k_flag_wave1) {
      static const uint8_t k_d_thr = k_waves_d_cnt;
      static const uint8_t k_e_thr = k_d_thr + k_waves_e_cnt;
      static const uint8_t k_f_thr = k_e_thr + k_waves_f_cnt;
      
      uint8_t idx = params.wave1;
      const float * const * table;
      
      if (idx < k_d_thr) {
        table = wavesD;
      }
      else if (idx < k_e_thr) {
        table = wavesE;
        idx -= k_d_thr;
      }
      else { // if (idx < k_f_thr) {
        table = wavesF;
        idx -= k_e_thr;
      }
      
      state.wave1 = table[idx];
    }
    if (flags & k_flag_subwave) {
      const uint8_t idx = params.subwave;
      state.subwave = wavesA[params.subwave];
    }
  }

  State       state;
  Params      params;
  dsp::BiQuad prelpf, postlpf;
};
