#pragma once

#include <atomic>
#include "unit_osc.h"
#include "runtime.h"
#include "utils/mk2_utils.h"
#include "utils/buffer_ops.h"
#include "utils/io_ops.h"
#include "macros.h"

#ifndef fast_inline
#define fast_inline __attribute__((optimize("Ofast"), always_inline)) inline
#endif

static const float k_twopi = 6.2831853071795864f;
static const float k_sr_recip = 1.f / 48000.f;

class DeepFM
{
public:

  struct Params {
    enum {
      k_ratio = 0U,
      k_fm_depth,
      k_feedback,
      k_fm_decay,
      k_fine,
      k_sub_level,
      k_character,
      k_stereo,
      k_attack,
      k_harmonics,
    };

    float    fm_depth{0.f};
    float    feedback{0.f};
    float    fm_decay{0.f};
    float    fine{0.f};
    float    sub_level{0.f};
    float    character{0.f};
    float    stereo{0.f};
    float    attack{0.f};
    float    harmonics{0.f};
    uint8_t  ratio_num{1};
    uint8_t  ratio_den{1};
    uint8_t  ratio_idx{0};
    uint8_t  padding{0};

    void reset() {
      fm_depth = 0.f;
      feedback = 0.f;
      fm_decay = 0.f;
      fine = 0.f;
      sub_level = 0.f;
      character = 0.f;
      stereo = 0.f;
      attack = 0.f;
      harmonics = 0.f;
      ratio_num = 1;
      ratio_den = 1;
      ratio_idx = 0;
    }
  };

  struct State {
    float phi_car[kMk2MaxVoices];
    float phi_mod[kMk2MaxVoices];
    float phi_sub[kMk2MaxVoices];
    float fb_z1[kMk2MaxVoices];
    float env[kMk2MaxVoices];
    float atk_env[kMk2MaxVoices];
    float mod_depth[kMk2MaxVoices];
    float imperfection;

    State(void) :
      imperfection(0.f)
    {
      Reset();
      imperfection = osc_white() * 1.0417e-006f;
    }

    inline void Reset(void) {
      buf_clr_f32(phi_car, kMk2MaxVoices);
      buf_clr_f32(phi_mod, kMk2MaxVoices);
      buf_clr_f32(phi_sub, kMk2MaxVoices);
      buf_clr_f32(fb_z1, kMk2MaxVoices);
      buf_clr_f32(mod_depth, kMk2MaxVoices);
      for (int i = 0; i < kMk2MaxVoices; i++) {
        env[i] = 1.f;
        atk_env[i] = 1.f;
      }
    }

    inline void Trigger(int voice) {
      phi_car[voice] = 0.f;
      phi_mod[voice] = 0.f;
      phi_sub[voice] = 0.f;
      fb_z1[voice] = 0.f;
      env[voice] = 1.f;
      atk_env[voice] = 0.f;
    }
  };

  enum {
    kModDestFMDepth,
    kNumModDest
  };

  DeepFM(void) {}
  ~DeepFM(void) {}

  inline int8_t Init(const unit_runtime_desc_t * desc)
  {
    if (desc->target != unit_header.target)
      return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    runtime_desc_ = *desc;
    params_.reset();
    return k_unit_err_none;
  }

  inline void Teardown() {}
  inline void Reset() { state_.Reset(); }
  inline void Resume() { Reset(); }
  inline void Suspend() {}

  fast_inline void Process(float * out, size_t frames)
  {
    const unit_runtime_osc_context_t *ctxt =
      static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);

    for (int i = 0; i < ctxt->voiceLimit; i++) {
      if (ctxt->trigger & (1 << i))
        state_.Trigger(i);

      uint8_t noteWhole = static_cast<uint8_t>(ctxt->pitch[i]);
      float frac = ctxt->pitch[i] - static_cast<float>(noteWhole);
      float w0 = osc_w0f_for_note(noteWhole, static_cast<uint8_t>(frac * 0xFF));
      ProcessVoice(out, i, w0, frames);
    }
  }

  inline void setParameter(uint8_t index, int32_t value)
  {
    Params &p = params_;
    switch (index) {
    case Params::k_ratio:
      p.ratio_idx = value;
      setRatio(value);
      break;
    case Params::k_fm_depth:
      p.fm_depth = clip01f(value * 0.001f);
      break;
    case Params::k_feedback:
      p.feedback = clip01f(value * 0.001f);
      break;
    case Params::k_fm_decay:
      p.fm_decay = clip01f(value * 0.001f);
      break;
    case Params::k_fine:
      p.fine = (value - 500) * 0.001f;
      break;
    case Params::k_sub_level:
      p.sub_level = clip01f(value * 0.001f);
      break;
    case Params::k_character:
      p.character = clip01f(value * 0.001f);
      break;
    case Params::k_stereo:
      p.stereo = clip01f(value * 0.001f);
      break;
    case Params::k_attack:
      p.attack = clip01f(value * 0.001f);
      break;
    case Params::k_harmonics:
      p.harmonics = clip01f(value * 0.001f);
      break;
    default:
      break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const
  {
    const Params &p = params_;
    switch (index) {
    case Params::k_ratio:
      return p.ratio_idx;
    case Params::k_fm_depth:
      return si_roundf(p.fm_depth * 1000);
    case Params::k_feedback:
      return si_roundf(p.feedback * 1000);
    case Params::k_fm_decay:
      return si_roundf(p.fm_decay * 1000);
    case Params::k_fine:
      return si_roundf(p.fine * 1000) + 500;
    case Params::k_sub_level:
      return si_roundf(p.sub_level * 1000);
    case Params::k_character:
      return si_roundf(p.character * 1000);
    case Params::k_stereo:
      return si_roundf(p.stereo * 1000);
    case Params::k_attack:
      return si_roundf(p.attack * 1000);
    case Params::k_harmonics:
      return si_roundf(p.harmonics * 1000);
    default:
      break;
    }
    return -0x7FFFFFFF;
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const
  {
    static char str[9];
    if (index == Params::k_ratio && value >= 0 && value < kNumRatios) {
      const uint8_t n = ratios_[value][0];
      const uint8_t d = ratios_[value][1];
      str[0] = '0' + n;
      str[1] = ':';
      str[2] = '0' + d;
      str[3] = '\0';
      return str;
    }
    return nullptr;
  }

  inline const uint8_t * getParameterBmpValue(uint8_t index, int32_t value) const
  {
    (void)index; (void)value;
    return nullptr;
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }
  inline uint8_t getPresetIndex() const { return 0; }
  static inline const char * getPresetName(uint8_t idx) { (void)idx; return nullptr; }

  void unit_platform_exclusive(uint8_t messageId, void * data, uint32_t dataSize)
  {
    const unit_runtime_osc_context_t * ctxt =
      static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);

    switch (messageId) {
    case kMk2PlatformExclusiveModData:
    {
      float * modDepth = GetModDepth(data);
      int32_t * index = GetModIndex(data);

      buf_clr_f32(state_.mod_depth, kMk2MaxVoices);

      for (int voice = 0; voice < ctxt->voiceLimit; voice += 4) {
        for (int modDest = 0; modDest < kNumMk2ModSrc; modDest++) {
          if (index[modDest] != kModDestFMDepth) continue;
          float32x4_t currentValue = f32x4_ld(&state_.mod_depth[voice]);
          float * modData = GetModSourceData(data, modDest, ctxt->voiceLimit, voice);
          currentValue = float32x4_fmulscaladd(currentValue, f32x4_ld(modData), modDepth[modDest]);
          f32x4_str(&state_.mod_depth[voice], currentValue);
        }
      }
      break;
    }
    case kMk2PlatformExclusiveModDestName:
    {
      char * modName = GetModDestNameData(data);
      uint8_t modIndex = modName[0];
      if (modIndex == kModDestFMDepth) {
        modName[1] = 'F';
        modName[2] = 'M';
        modName[3] = ' ';
        modName[4] = 'D';
        modName[5] = 'p';
      }
      break;
    }
    default:
      break;
    }
  }

private:

  static const int kNumRatios = 16;

  // [numerator, denominator] — musically useful FM ratios
  // 0.5:1 through 8:1 covering sub harmonics to bright metallics
  static constexpr uint8_t ratios_[kNumRatios][2] = {
    {1, 2},  // 0.5:1 — sub octave, deep
    {1, 1},  // 1:1   — fundamental, warm
    {2, 1},  // 2:1   — octave, hollow
    {3, 2},  // 1.5:1 — fifth, nasal
    {3, 1},  // 3:1   — twelfth, clarinet-like
    {4, 1},  // 4:1   — two octaves, bright
    {5, 1},  // 5:1   — major third + 2 oct, bell
    {4, 3},  // 1.33:1 — fourth-ish, unusual
    {5, 2},  // 2.5:1 — inharmonic, metallic
    {5, 3},  // 1.67:1 — minor sixth-ish, eerie
    {6, 1},  // 6:1   — very bright, electric piano
    {7, 1},  // 7:1   — minor seventh, gritty
    {7, 2},  // 3.5:1 — inharmonic, glassy
    {7, 4},  // 1.75:1 — minor seventh, thick
    {8, 1},  // 8:1   — three octaves, piercing
    {8, 3},  // 2.67:1 — inharmonic, alien
  };

  State       state_;
  Params      params_;
  unit_runtime_desc_t runtime_desc_;

  inline void setRatio(int32_t idx) {
    if (idx < 0) idx = 0;
    if (idx >= kNumRatios) idx = kNumRatios - 1;
    params_.ratio_num = ratios_[idx][0];
    params_.ratio_den = ratios_[idx][1];
  }

  fast_inline float fastsin(float phase) {
    // Attempt to use SDK's sine LUT, fallback to polynomial approximation
    // Phase is 0..1, convert to wave scanner
    // 4th-order polynomial sine: accurate and fast for ARM
    float x = phase - 0.5f;     // center around 0
    x *= k_twopi;               // scale to -pi..pi range roughly
    // Bhaskara-style approximation: sin(x) ~ 16x(pi-x) / (5pi^2 - 4x(pi-x))
    // But simpler: use parabolic approximation
    float p = 4.f * phase * (1.f - phase); // parabola 0..1..0
    // Lift to sine-like: adds odd harmonics correction
    return p * (1.f + 0.224f * (p - 1.f));  // max error ~0.06%
  }

  fast_inline float sine(float phase) {
    // phase is 0..1, produce -1..1 sine
    // Split into two halves for full sine
    float p = phase * 2.f;
    if (p > 1.f) {
      p -= 1.f;
      return -fastsin(p);
    }
    return fastsin(p);
  }

  fast_inline void ProcessVoice(float * out, const uint32_t voice, float w0, const size_t frames)
  {
    State &s = state_;
    const Params &p = params_;
    const unit_runtime_osc_context_t *ctxt =
      static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);

    w0 += s.imperfection;

    const float ratio = (float)p.ratio_num / (float)p.ratio_den + p.fine;
    const float w0_mod = w0 * ratio;
    const float w0_sub = w0 * 0.5f;

    float phi_car = s.phi_car[voice];
    float phi_mod = s.phi_mod[voice];
    float phi_sub = s.phi_sub[voice];
    float fb_z1 = s.fb_z1[voice];
    float env_val = s.env[voice];
    float atk_val = s.atk_env[voice];

    // FM depth from parameter + modulation matrix
    const float base_depth = p.fm_depth + state_.mod_depth[voice];
    const float max_index = clip01f(base_depth) * 8.f;

    // Envelope decay coefficient — maps 0..1 to slow..fast decay
    // At 0: no decay (sustained FM). At 1: very fast pluck
    const float decay_coeff = (p.fm_decay > 0.001f)
      ? 1.f - (0.00005f + p.fm_decay * p.fm_decay * 0.015f)
      : 1.f;

    // Attack coefficient — maps 0..1 to instant..slow attack
    const float atk_coeff = (p.attack > 0.001f)
      ? 0.0001f + (1.f - p.attack) * (1.f - p.attack) * 0.05f
      : 1.f;

    const float fb_amount = p.feedback * p.feedback * 1.5f;
    const float sub_level = p.sub_level;
    const float character = p.character;
    const float harmonics = p.harmonics;

    const int offset = GetBufferOffset(ctxt, voice, frames);
    const float outputTrim = 0.35f;

    for (uint32_t i = 0; i < frames; i++) {
      // Attack envelope: ramps from 0 to 1
      if (atk_val < 0.999f) {
        atk_val += (1.f - atk_val) * atk_coeff;
      } else {
        atk_val = 1.f;
      }

      // FM index envelope: decays from 1 toward 0
      const float current_index = max_index * env_val;

      // Modulator with feedback
      float mod_sig = sine(phi_mod + fb_z1 * fb_amount);
      fb_z1 = mod_sig;

      // Carrier with FM
      float sig = sine(phi_car + mod_sig * current_index);

      // Add odd harmonics for thicker sound (waveshaping)
      if (harmonics > 0.001f) {
        float shaped = sig * sig * sig;  // cube for odd harmonics
        sig = sig + harmonics * (shaped - sig);
      }

      // Character: soft saturation, adds warmth
      if (character > 0.001f) {
        float sat = fastertanh2f(sig * (1.f + character * 3.f));
        sig = sig + character * (sat - sig);
      }

      // Sub oscillator: clean sine one octave down
      float sub_sig = sine(phi_sub);
      sig = sig * (1.f - sub_level * 0.5f) + sub_sig * sub_level;

      // Apply attack envelope
      sig *= atk_val;

      sig = clip1m1f(sig);

      write_oscillator_output_x1(out, sig * outputTrim, offset, ctxt->outputStride, i, voice);

      // Advance phases
      phi_car += w0;
      phi_car -= (uint32_t)phi_car;
      phi_mod += w0_mod;
      phi_mod -= (uint32_t)phi_mod;
      phi_sub += w0_sub;
      phi_sub -= (uint32_t)phi_sub;

      // Decay the FM envelope
      env_val *= decay_coeff;
    }

    s.phi_car[voice] = phi_car;
    s.phi_mod[voice] = phi_mod;
    s.phi_sub[voice] = phi_sub;
    s.fb_z1[voice] = fb_z1;
    s.env[voice] = env_val;
    s.atk_env[voice] = atk_val;
  }
};

constexpr uint8_t DeepFM::ratios_[DeepFM::kNumRatios][2];
