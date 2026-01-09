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
 *  File: waves.h
 *
 *  Simple Morphing Wavetable Demo Oscillator
 *
 */

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <climits>

#include "processor.h"
#include "unit_osc.h"
#include "waves_common.h"
#include "dsp/biquad.hpp"

class Osc : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; } // NTS-1 osc do not support sdram allocation

  // audio parameters
  struct Params
  {
    enum
    {
      k_shape = 0U,
      k_sub_mix,
      k_wave_a,
      k_wave_b,
      k_sub_wave,
      k_ring_mix,
      k_bit_crush,
      k_drift,
    };

    float sub_mix;
    float ring_mix;
    float bit_crush;
    float shape;
    float drift;
    uint8_t wave_a;
    uint8_t wave_b;
    uint8_t sub_wave;

    void reset()
    {
      sub_mix = 0.05f;
      ring_mix = 0.f;
      bit_crush = 0.f;
      shape = 0.f;
      drift = 1.25f;
      wave_a = 0;
      wave_b = 0;
      sub_wave = 0;
    }

    Params() { reset(); }
  };

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    Params &p = params_;
    State &s = state_;

    switch (index)
    {
    case Params::k_shape:
      //  min, max,  center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   1023, 0,      0,       k_unit_param_type_none, 0,    0,          0,          {"SHAPE"}},
      p.shape = 0.005f + param_10bit_to_f32(value) * 0.99f;
      break;

    case Params::k_sub_mix:
      //  min, max,  center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   1023, 0,      0,       k_unit_param_type_none, 0,    0,          0,          {"SUB"}},
      p.sub_mix = 0.05f + param_10bit_to_f32(value) * 0.90f;
      break;

    case Params::k_wave_a:
    {
      //  min, max,          center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   WAVE_A_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"WAVE A"}},
      p.wave_a = value % WAVE_A_CNT;
      s.flags.fetch_or(State::k_flag_wave_a);
    }
    break;

    case Params::k_wave_b:
    {
      //  min, max,          center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   WAVE_B_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"WAVE B"}},
      p.wave_b = value % WAVE_B_CNT;
      s.flags.fetch_or(State::k_flag_wave_b);
    }
    break;

    case Params::k_sub_wave:
      //  min, max,            center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   SUB_WAVE_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"SUB WAVE"}},
      p.sub_wave = value % SUB_WAVE_CNT;
      s.flags.fetch_or(State::k_flag_sub_wave);
      break;

    case Params::k_ring_mix:
      //  min, max,  center, default, type,                      frac, frac. mode, <reserved>, name
      // {0,   1000,  0,      0,       k_unit_param_type_percent, 1,    1,          0,          {"RING MIX"}},
      p.ring_mix = clip01f(value * 0.001f);
      break;

    case Params::k_bit_crush:
      //  min, max,  center, default, type,                      frac, frac. mode, <reserved>, name
      // {0,   1000,  0,      0,       k_unit_param_type_percent, 1,    1,          0,          {"BIT CRUSH"}},
      p.bit_crush = clip01f(value * 0.001f);
      s.flags.fetch_or(State::k_flag_bit_crush);
      break;

    case Params::k_drift:
      //  min, max,  center, default, type,                      frac, frac. mode, <reserved>, name
      // {0,   1000,  0,      250,     k_unit_param_type_percent, 1,    1,          0,          {"DRIFT"}},
      p.drift = 1.f + value * 0.001f;
      break;

    default:
      break;
    }
  }

  void init(float *) override final
  {
    // initialize pre/post filter coefficients
    prelpf_.mCoeffs.setPoleLP(0.9f);
    postlpf_.mCoeffs.setFOLP(osc_tanpif(0.45f));

    params_.reset();
  }

  void reset() override final
  {
    state_.reset();
  }

  void resume() override final
  {
    state_.reset();
  }

  // set frequency in digital w (w = f/samplerate, 0.5 is Nyquist)
  void setPitch(float w0)
  {
    updatePitch(w0);
  }

  // lfo in (-1.0f, 1.0f)
  void setShapeLfo(float lfo)
  {
    state_.lfo = lfo;
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    State &s = state_;
    const Params p = params_;

    // Handle events.
    {
      const uint32_t flags = s.flags.exchange(State::k_flags_none, std::memory_order_relaxed);
      updateWaves(flags);

      if (flags & State::k_flag_reset)
        s.reset();

      if (flags & State::k_flag_bit_crush)
      {
        s.dither = p.bit_crush * 2e-008f;
        s.bit_res = osc_bitresf(p.bit_crush);
        s.bit_res_recip = 1.f / s.bit_res;
      }
    }

    // Temporaries.
    float phi_a = s.phi_a;
    float phi_b = s.phi_b;
    float phi_sub = s.phi_sub;

    float lfoz = s.lfoz;
    const float lfo_inc = (s.lfo - lfoz) / frames;

    const float ditheramt = p.bit_crush * 2e-008f;

    const float sub_mix = p.sub_mix * 0.5011872336272722f;
    const float ring_mix = p.ring_mix;

    for (const float *out_end = out + frames; out != out_end; in += 2, out += 1)
    {
      const float wave_mix = clip01f(p.shape + lfoz);

      float sig = (1.f - wave_mix) * osc_wave_scanf(s.wave_a, phi_a);
      sig += wave_mix * osc_wave_scanf(s.wave_b, phi_b);

      const float sub_sig = osc_wave_scanf(s.sub_wave, phi_sub);
      sig = (1.f - ring_mix) * sig + ring_mix * 1.4125375446227544f * (sub_sig * sig);
      sig += sub_mix * sub_sig;
      sig *= 1.4125375446227544f;
      sig = clip1m1f(fastertanh2f(sig));

      sig = prelpf_.process_fo(sig);
      sig += s.dither * osc_white();
      sig = si_roundf(sig * s.bit_res) * s.bit_res_recip;
      sig = postlpf_.process_fo(sig);

      // write output
      out[0] = sig;

      phi_a += s.w0_a;
      phi_a -= (uint32_t)phi_a;
      phi_b += s.w0_b;
      phi_b -= (uint32_t)phi_b;
      phi_sub += s.w0_sub;
      phi_sub -= (uint32_t)phi_sub;
      lfoz += lfo_inc;
    }

    // Update state
    s.phi_a = phi_a;
    s.phi_b = phi_b;
    s.phi_sub = phi_sub;
    s.lfoz = lfoz;
  }

  inline void allNoteOff() override final
  {
    // Schedule phase reset
    state_.flags.fetch_or(State::k_flag_reset);
  }

private:
  struct State
  {

    enum
    {
      k_flags_none = 0,
      k_flag_wave_a = 1 << 1,
      k_flag_wave_b = 1 << 2,
      k_flag_sub_wave = 1 << 3,
      k_flag_ring_mix = 1 << 4,
      k_flag_bit_crush = 1 << 5,
      k_flag_reset = 1 << 6
    };

    const float *wave_a = wavesA[0];               // selected wave a data
    const float *wave_b = wavesD[0];               // selected wave b data
    const float *sub_wave = wavesA[0];             // selected sub wave data
    float phi_a = 0.f;                             // wave a phase
    float phi_b = 0.f;                             // wave b phase
    float phi_sub = 0.f;                           // sub wave phase
    float w0_a = 440.f * k_samplerate_recipf;      // wave a phase increment
    float w0_b = 440.f * k_samplerate_recipf;      // wave b phase increment
    float w0_sub = 220.f * k_samplerate_recipf;    // sub wave phase increment
    float lfo = 0.f;                               // target lfo value
    float lfoz = 0.f;                              // current interpolated lfo value
    float dither = 0.f;                            // dithering amount before bit reduction
    float bit_res = 1.f;                           // bit depth scaling factor
    float bit_res_recip = 1.f;                     // bit depth scaling reciprocal, returns signal to 0.-1.f after scaling/rounding
    float imperfection;                            // tuning imperfection
    std::atomic_uint_fast32_t flags{k_flags_none}; // flags passed to audio processing thread

    State(void)
    {
      reset();
      imperfection = osc_white() * 1.0417e-006f; // +/- 0.05Hz@48KHz
    }

    inline void reset(void)
    {
      phi_a = 0.f;
      phi_b = 0.f;
      phi_sub = 0.f;
      lfo = lfoz;
    }
  };

  State state_;
  Params params_;
  dsp::BiQuad prelpf_, postlpf_;

  float w0_;
  float lfo_;

  fast_inline void updatePitch(float w0)
  {
    w0 += state_.imperfection;
    const float drift = params_.drift;
    state_.w0_a = w0;
    // Alt. osc with slight phase drift (0.25Hz@48KHz)
    state_.w0_b = w0 + drift * 5.20833333333333e-006f;
    // Sub one octave down, with a phase drift (0.15Hz@48KHz)
    state_.w0_sub = 0.5f * w0 + drift * 3.125e-006f;
  }

  fast_inline void updateWaves(const uint32_t flags)
  {
    if (flags & State::k_flag_wave_a)
    {
      static const uint8_t k_a_thr = k_waves_a_cnt;
      static const uint8_t k_b_thr = k_a_thr + k_waves_b_cnt;
      static const uint8_t k_c_thr = k_b_thr + k_waves_c_cnt;

      uint8_t idx = params_.wave_a;
      const float *const *table;

      if (idx < k_a_thr)
      {
        table = wavesA;
      }
      else if (idx < k_b_thr)
      {
        table = wavesB;
        idx -= k_a_thr;
      }
      else
      {
        table = wavesC;
        idx -= k_b_thr;
      }
      state_.wave_a = table[idx];
    }
    if (flags & State::k_flag_wave_b)
    {
      static const uint8_t k_d_thr = k_waves_d_cnt;
      static const uint8_t k_e_thr = k_d_thr + k_waves_e_cnt;
      static const uint8_t k_f_thr = k_e_thr + k_waves_f_cnt;

      uint8_t idx = params_.wave_b;
      const float *const *table;

      if (idx < k_d_thr)
      {
        table = wavesD;
      }
      else if (idx < k_e_thr)
      {
        table = wavesE;
        idx -= k_d_thr;
      }
      else
      { // if (idx < k_f_thr) {
        table = wavesF;
        idx -= k_e_thr;
      }

      state_.wave_b = table[idx];
    }
    if (flags & State::k_flag_sub_wave)
    {
      const uint8_t idx = params_.sub_wave;
      state_.sub_wave = wavesA[params_.sub_wave];
    }
  }
};
