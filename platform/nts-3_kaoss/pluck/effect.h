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

//*/

/*
 *  File: effect.h
 *
 *  Polyphonic Karplus-Strong waveguide pluck synthesizer for NTS-3 kaoss pad kit.
 *
 */

#include <array>
#include <cmath>

#include "processor.h"
#include "unit_genericfx.h"
#include "osc_api.h"

#include "dsp/utility.hpp"
#include "dsp/delayline.hpp"
#include "dsp/fir.hpp"
#include "dsp/one_pole.hpp"
#include "dsp/dc_blocker.hpp"
#include "dsp/thiran_allpass.hpp"
#include "dsp/voice_allocator.hpp"

inline float overdrive(float x, float drive)
{
  const float d = clampf(drive, 0.f, 1.f);
  const float pre_gain = d * d * d * 24.f;
  const float distorted = fast_tanh(x * pre_gain);
  const float mix = d * (2.f - d);
  return lerpf(x, distorted, mix);
}

// ---- Shared parameter block -------------------------------------------------------

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
    damp = 0.f;
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

// ---- Single Karplus-Strong waveguide voice ----------------------------------------

/**
 * @brief One Karplus-Strong waveguide string.
 * @author Shijie Xia (xiashj@korg.co.jp)
 *
 * External SDRAM buffer must be assigned via set_memory() before use.
 */
class Waveguide
{
public:
  static constexpr size_t N = 4096;
  static constexpr size_t M_DISPERSION = 8;

  void set_memory(float *mem)
  {
    delay.set_memory(mem);
  }

  void init(float samplerate)
  {
    sr = samplerate;
    damp_filter.reset();
    noise_filter.reset();
    dc_blocker.reset(sr);
    dispersion_filter.reset();
    curved_bridge = 0.f;
  }

  void pluck(float hz, const Params &p)
  {
    pitch = hz;

    delay.clear();
    damp_filter.reset();
    noise_filter.reset();
    curved_bridge = 0.f;
    dispersion_filter.reset();

    noise_filter.set_lp(p.noise_cutoff / sr);

    const float string_len = compute_string_len_samples(pitch);
    for (size_t i = 0; i < static_cast<size_t>(string_len) + 1; ++i)
    {
      float noise = osc_white();
      noise = noise_filter.process_sample(noise);
      delay.write(noise);
    }
  }

  // Returns output sample before overdrive.
  float process_sample(const Params &p, float input_mono)
  {
    const auto [a1, dc_delay_samples] = compute_allpass(pitch, p.dispersion);
    dispersion_filter.set_coeff(a1);

    const float string_len = compute_string_len_samples(pitch, dc_delay_samples);

    damp_filter.set_damp(p.damp);

    const float rt60_samples = 0.07f * std::pow(2.f, p.decay * 8.f) * sr;
    const float w = 2.f * M_PI / (string_len + 1.f);
    const float fir_gain = damp_filter.magnitude_at(w);
    const float exponent = std::max(-10.f * string_len / rt60_samples, -127.f / 12.f);
    const float gain = std::min(std::pow(2.f, exponent) / std::max(fir_gain, 0.001f), 1.f);

    noise_filter.set_lp(p.noise_cutoff / sr);

    const float comb_delay_samples = 0.5f * p.pickup_pos * sr / pitch;
    const float bridge_amount = p.stiffness * p.stiffness * 0.01f;

    float string_len_modulated = string_len * (1.f - curved_bridge * bridge_amount);
    string_len_modulated = string_len_modulated * (1.f + osc_white() * p.noise_fm_amount * 0.025f);

    const float delay_out = delay.read_lagrange_2nd(string_len_modulated);

    float y = delay_out;
    if (comb_delay_samples > 1.f)
      y -= delay.read_linear(comb_delay_samples);

    curved_bridge = compute_curved_bridge(y);

    // === feedback loop ===
    float v = delay_out + input_mono;
    v = dc_blocker.process_sample(v);
    v = damp_filter.process_sample(v);
    if (p.dispersion > 0.f)
      v = dispersion_filter.process_sample(v);
    v *= gain;
    delay.write(v);

    return y;
  }

  void mute() { delay.clear(); }

  float pitch = 440.f;

private:
  float compute_string_len_samples(float pitch_hz, float extra_delay = 0.f) const
  {
    const float delay_samples = sr / pitch_hz - 1.f - extra_delay;
    return clampf(delay_samples, 1.f, static_cast<float>(N - 2));
  }

  inline float compute_curved_bridge(float x) const
  {
    // inspired by the design of Sitar bridge
    // courtesy to Emilie Gillet (Mutable Instruments Rings)
    // https://github.com/pichenettes/eurorack/blob/08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4/rings/dsp/string.cc#L184
    float sign = x > 0.f ? 1.f : -1.5f;
    x = std::abs(x) - 0.025f;
    return (std::abs(x) + x) * sign;
  }

  struct AllpassParams
  {
    float a1;
    float dc_delay_samples;
  };

  AllpassParams compute_allpass(float pitch_hz, float dispersion) const
  {
    if (dispersion <= 0.f)
      return {0.f, 0.f};

    const float string_len_raw = sr / pitch_hz - 1.f;
    const float D_max = clampf(0.5f * string_len_raw / static_cast<float>(M_DISPERSION), 1.f, 20.f);
    const float D = lerpf(1.f, D_max, dispersion);
    const float a1 = (1.f - D) / (1.f + D);
    return {a1, static_cast<float>(M_DISPERSION) * D};
  }

  float sr = 48000.f;
  DelayLine<N> delay;
  SymmetricFir3 damp_filter;
  OnePole noise_filter;
  DcBlocker dc_blocker;
  ThiranAllpassCascade<M_DISPERSION> dispersion_filter;
  float curved_bridge = 0.f;
};

// ---- Polyphonic effect (NUM_VOICES waveguides + round-robin allocator) -------------

/**
 * @brief Polyphonic Karplus-Strong for NTS-3 kaoss pad kit.
 * @author Shijie Xia (xiashj@korg.co.jp)
 *
 * Touch X maps continuously to pitch (C1–C5, 4 octaves).
 * Touch Y maps to damp (bottom=bright, top=dark) via default_mappings.
 * Each phase_began strikes a new voice in round-robin order.
 */
class Effect : public Processor
{
public:
  static constexpr size_t NUM_VOICES = 4;

  uint32_t getBufferSize() const override final { return NUM_VOICES * Waveguide::N; }

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

  inline void setParameter(uint8_t index, int32_t value) override final
  {
    switch (index)
    {
    case DAMP:
      params.damp = param_10bit_to_f32(value);
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

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    (void)index;
    (void)value;
    return nullptr;
  }

  void init(float *allocated_buffer) override final
  {
    buffer = allocated_buffer;
    for (size_t i = 0; i < NUM_VOICES; ++i)
    {
      voices[i].set_memory(buffer + i * Waveguide::N);
      voices[i].init(getSampleRate());
    }
    params.reset();
  }

  void teardown() override final { buffer = nullptr; }

  void process(const float *__restrict in, float *__restrict out, uint32_t frames) override final
  {
    const Params p = params;

    for (const float *out_end = out + frames * 2; out != out_end; in += 2, out += 2)
    {
      const float input_mono = (in[0] + in[1]) * 0.5f / NUM_VOICES;

      float mix = 0.f;
      for (auto &v : voices)
        mix += v.process_sample(p, input_mono);
      mix /= static_cast<float>(NUM_VOICES);

      // overdrive applied to the mix of all voices
      const float y = overdrive(mix, p.drive);
      out[0] = y;
      out[1] = y;
    }
  }

  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) override final
  {
    (void)id;

    if (phase == k_unit_touch_phase_ended || phase == k_unit_touch_phase_cancelled)
    {
      for (auto &v : voices)
        v.mute();
      last_col = last_row = UINT32_MAX;
      return;
    }

    // Tonnetz grid: 12 cols × 8 rows
    // note = BASE_NOTE + col·1 + row·5  (semitones, like a bass neck)
    const uint32_t col = (x * COLS) / 1024;
    const uint32_t row = (y * ROWS) / 1024;

    if (phase == k_unit_touch_phase_began || col != last_col || row != last_row)
    {
      pluck_note(static_cast<uint8_t>(BASE_NOTE + col + row * 5));
      last_col = col;
      last_row = row;
    }
  }

  void pluck_note(uint8_t note)
  {
    const size_t slot = allocator.note_on(note);
    voices[slot].pluck(note_to_hz(note), params);
  }

  static constexpr uint32_t COLS      = 12;
  static constexpr uint32_t ROWS      = 8;
  static constexpr uint8_t  BASE_NOTE = 24; // C1; grid spans C1–Bb4 (MIDI 24–70)

  float *buffer = nullptr;
  Params params;
  std::array<Waveguide, NUM_VOICES> voices;
  VoiceAllocator<NUM_VOICES> allocator;
  uint32_t last_col = UINT32_MAX;
  uint32_t last_row = UINT32_MAX;
};
