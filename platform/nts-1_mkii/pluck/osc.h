#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2026, KORG INC.
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

*/

#include "processor.h"
#include "unit_osc.h"

/**
 * @brief karplus-strong waveguide synthesis
 * @author Shijie Xia (xiashj@korg.co.jp)
 */

#include "dsp/utility.hpp"
#include "dsp/delayline.hpp"
#include "dsp/fir.hpp"
#include "dsp/one_pole.hpp"
#include "dsp/dc_blocker.hpp"
#include "dsp/thiran_allpass.hpp"

inline float overdrive(float x, float drive)
{
  const float d = clampf(drive, 0.f, 1.f);
  const float pre_gain = d * d * d * 24.f;
  const float distorted = fast_tanh(x * pre_gain);
  const float mix = d * (2.f - d);
  return lerpf(x, distorted, mix);
}

class Osc : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; } // ununsed
  void setPitch(float w0) {}                                  // ununsed
  void setShapeLfo(float lfo) {}                              // ununsed

  // audio parameters
  enum
  {
    DAMP = 0U,
    DECAY,
    NOISE_CUTOFF,
    PICKUP_POS,
    DRIVE,
    STIFFNESS,
    NOISE_FM,
    DISPERSION,
    NUM_PARAMS
  };

  // Note: Make sure that default param values correspond to declarations in header.c
  struct Params
  {
    float damp;
    float decay;
    float noise_cutoff;
    float pickup_pos;
    float drive;
    float stiffness;
    float noise_fm_amount;
    float dispersion;

    void reset()
    {
      damp = 0.5f;
      decay = 1.f;
      noise_cutoff = 1.f;
      pickup_pos = 0.f;
      drive = 0.f;
      stiffness = 0.f;
      noise_fm_amount = 0.f;
      dispersion = 0.f;
    }

    Params() { reset(); }
  };

  void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case DAMP:
      params.damp = param_10bit_to_f32(value); // 0 .. 1023 -> 0.0 .. 1.0
      break;

    case DECAY:
      params.decay = param_10bit_to_f32(value);
      break;

    case NOISE_CUTOFF:
      params.noise_cutoff = norm_to_freq(param_10bit_to_f32(value));
      break;

    case PICKUP_POS:
      params.pickup_pos = param_10bit_to_f32(value);
      break;

    case DRIVE:
      params.drive = param_10bit_to_f32(value);
      break;

    case STIFFNESS:
      params.stiffness = param_10bit_to_f32(value);
      break;

    case NOISE_FM:
      params.noise_fm_amount = param_10bit_to_f32(value);
      break;

    case DISPERSION:
      params.dispersion = param_10bit_to_f32(value);
      break;

    default:
      break;
    }
  }

  void init(float *) override final
  {
    params.reset();
    damp_filter.reset();
    noise_filter.reset();
    curved_bridge = 0.f;
    dispersion_filter.reset();
    dc_blocker.reset(getSampleRate());
  }

  void noteOn(uint8_t note, uint8_t velocity) override final
  {
    pitch = note_to_hz(note);

    // reset every filter inside the loop
    delay.clear();
    damp_filter.reset();
    noise_filter.reset();
    curved_bridge = 0.f;
    dispersion_filter.reset();

    // fill one period of noise
    const float string_len = compute_string_len_samples(pitch);
    for (size_t i = 0; i < static_cast<size_t>(string_len) + 1; ++i)
    {
      float noise = osc_white();
      noise = noise_filter.process_sample(noise);
      delay.write(noise);
    }
  }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    // Caching current parameter values. Consider smoothing sensitive parameters in audio loop
    const Params p = params;

    // dispersion: first-order Thiran allpass cascade with loop-delay compensation
    const auto [a1, dc_delay_samples] = compute_allpass(pitch, p.dispersion);
    dispersion_filter.set_coeff(a1);

    const float string_len = compute_string_len_samples(pitch, dc_delay_samples);

    // damp filter
    damp_filter.set_damp(p.damp);

    // decay time
    const float rt60_samples = 0.07f * std::pow(2.f, p.decay * 8.f) * getSampleRate();
    const float w = 2 * M_PI / (string_len + 1.f);
    const float fir_gain = damp_filter.magnitude_at(w);
    const float exponent = std::max(-10.f * string_len / rt60_samples, -127.f / 12.f);
    const float gain = std::min(std::pow(2.f, exponent) / std::max(fir_gain, 0.001f), 1.f);

    // input noise cutoff
    noise_filter.set_lp(p.noise_cutoff / getSampleRate());

    // pickup comb filter
    const float comb_delay_samples = 0.5f * p.pickup_pos * getSampleRate() / pitch;

    // stiffness: [0, 1] -> [0, 0.01]
    const float bridge_amount = p.stiffness * p.stiffness * 0.01f;

    for (const float *out_end = out + frames; out != out_end; in += 2, out += 1)
    {
      // curved bridge shortens the string by 1% maximum
      float string_len_modulated = string_len * (1 - curved_bridge * bridge_amount);

      // noise FM on string length
      string_len_modulated = string_len_modulated * (1.f + osc_white() * p.noise_fm_amount * 0.025f);

      // read delayline with modulated length
      const float delay_out = delay.read_lagrange_2nd(string_len_modulated);

      // post loop output path
      float y = delay_out;

      // apply comb filter outside the loop
      if (comb_delay_samples > 1.f)
      {
        y -= delay.read_linear(comb_delay_samples);
      }

      // update curved bridge from output
      curved_bridge = compute_curved_bridge(y);

      y = overdrive(y, p.drive);

      out[0] = y;

      // === feedback loop start ===
      float v = delay_out;
      
      // dc blocker
      v = dc_blocker.process_sample(v);

      // damp filter (3-tap FIR)
      v = damp_filter.process_sample(v);

      // dispersion allpass filter
      if (p.dispersion > 0.f)
        v = dispersion_filter.process_sample(v);

      // gain adjustment
      v *= gain;

      // write back
      delay.write(v);
      // === feedback loop end ===
    }
  }

private:
  Params params;
  float pitch = 440.f;

  float compute_string_len_samples(float pitch_hz, float extra_delay = 0.f) const
  {
    // subract the extra 1 sample delay introduced by 3-tap FIR damping filter
    const float delay_samples = getSampleRate() / pitch_hz - 1.f - extra_delay;
    return clampf(delay_samples, 1.f,
                  static_cast<float>(N - 2)); // leave enough margin for lagrange interpolation
  }

  static constexpr size_t N = 4096;
  DelayLine<N> delay;

  SymmetricFir3 damp_filter;
  OnePole noise_filter;
  DcBlocker dc_blocker;

  float curved_bridge = 0.f;

  inline float compute_curved_bridge(float x) const
  {
    // inspired by the design of Sitar bridge
    // courtesy to Emilie Gillet (Mutable Instruments Rings)
    // https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/rings/dsp/string.cc#L184
    float sign = x > 0.f ? 1.f : -1.5f;
    x = std::abs(x) - 0.025f; // asymmetric
    return (std::abs(x) + x) * sign;
  }

  static constexpr size_t M_DISPERSION = 8;
  ThiranAllpassCascade<M_DISPERSION> dispersion_filter;

  struct AllpassParams
  {
    float a1;
    float dc_delay_samples;
  };

  // Maps dispersion [0,1] to a Thiran first-order allpass coefficient and its DC group delay.
  // D per filter: dispersion=0 → D=1 (a1=0, no effect); dispersion=1 → D=D_max.
  // D_max ≤ half the raw loop length so the compensated string length stays positive.
  AllpassParams compute_allpass(float pitch_hz, float dispersion) const
  {
    if (dispersion <= 0.f)
      return {0.f, 0.f};

    const float string_len_raw = getSampleRate() / pitch_hz - 1.f;
    const float D_max = clampf(0.5f * string_len_raw / static_cast<float>(M_DISPERSION), 1.f, 20.f);
    const float D = lerpf(1.f, D_max, dispersion);
    const float a1 = (1.f - D) / (1.f + D); // Thiran: GD(DC) = D exactly; D ≥ 1 → a1 ≤ 0
    return {a1, static_cast<float>(M_DISPERSION) * D};
  }
};
