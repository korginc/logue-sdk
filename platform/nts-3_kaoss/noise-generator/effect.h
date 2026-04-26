#pragma once
/*
 *  File: effect.h
 *
 *  Noise Generator logic for NTS-3 kaoss pad kit
 *
 */

#include <atomic>
#include <climits>
#include <cstddef>
#include <cstdint>

#include "osc_api.h"
#include "unit_genericfx.h"
#include "utils/float_math.h"

class NoiseGenerator {
public:
  enum {
    LEVEL = 0U,
    PAN,
    TYPE,
    DRYWET,
    AUTOPAN,
    APAN_SPD,
    APAN_SYNC,
    NUM_PARAMS
  };

  enum NoiseType { WHITE = 0, PINK, BROWN, BLUE, VIOLET, NUM_TYPES };

  struct Params {
    float level{0.5f};
    float pan{0.5f};
    uint8_t type{WHITE};
    float drywet{1.0f};
    float autopan_depth{0.0f};
    float autopan_speed{0.5f};
    uint8_t autopan_sync{0};

    void reset() {
      level = 0.5f;
      pan = 0.5f;
      type = WHITE;
      drywet = 1.0f;
      autopan_depth = 0.0f;
      autopan_speed = 0.5f;
      autopan_sync = 0;
    }
  };

  NoiseGenerator(void) {}
  ~NoiseGenerator(void) {}

  inline int8_t Init(const unit_runtime_desc_t *desc) {
    if (!desc)
      return k_unit_err_undef;
    if (desc->target != unit_header.common.target)
      return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    params_.reset();
    reset_state();

    return k_unit_err_none;
  }

  inline void Teardown() {}

  inline void Reset() { is_active_ = true; }

  inline void Resume() { is_active_ = true; }

  inline void Suspend() { is_active_ = false; }

  inline void setTempo(uint32_t tempo) {
    // Tempo is passed in UQ16.16 representation
    float bpm = (float)tempo / 65536.0f;
    if (bpm < 1.0f)
      bpm = 1.0f;
    bpm_ = bpm;
  }

  inline void reset_state() {
    pink_b0 = pink_b1 = pink_b2 = 0.f;
    brown_state = 0.f;
    last_white = 0.f;
    amp_ = 0.f;
    autopan_phase_ = 0.f;
    is_active_ = false;
    is_touched_ = false;
  }

  fast_inline void Process(const float *in, float *out, size_t frames) {
    const float *__restrict in_p = in;
    float *__restrict out_p = out;
    const float *out_e = out_p + (frames << 1);

    const Params p = params_;

    const float target_amp = (is_active_ && is_touched_) ? p.level : 0.f;

    // Gain compensation constants for balanced perceived volume
    static const float k_gains[] = {
        1.0f, // White
        0.3f, // Pink
        0.7f, // Brown
        0.7f, // Blue
        0.5f  // Violet
    };
    const float type_gain = k_gains[p.type < NUM_TYPES ? p.type : WHITE];

    const float wet = p.drywet;
    const float dry = 1.0f - wet;

    // Calculate LFO frequency for Autopan
    float lfo_freq = 0.0f;
    if (p.autopan_sync == 0) {
      // Off: 0.1Hz to 10.0Hz
      lfo_freq = 0.1f + p.autopan_speed * 9.9f;
    } else {
      // Tempo sync
      float beats_per_sec = bpm_ / 60.0f;
      float multiplier = 1.0f;
      switch (p.autopan_sync) {
      case 1:
        multiplier = 0.25f;
        break; // 1/1
      case 2:
        multiplier = 0.5f;
        break; // 1/2
      case 3:
        multiplier = 1.0f;
        break; // 1/4
      case 4:
        multiplier = 2.0f;
        break; // 1/8
      case 5:
        multiplier = 4.0f;
        break; // 1/16
      case 6:
        multiplier = 8.0f;
        break; // 1/32
      }
      lfo_freq = beats_per_sec * multiplier;
    }

    const float lfo_inc = lfo_freq / 48000.0f;
    const float lfo_depth = p.autopan_depth;

    for (; out_p != out_e; in_p += 2, out_p += 2) {
      float w = osc_white();
      float noise = 0.f;

      switch (p.type) {
      case PINK:
        // Paul Kellet's economy 3-pole filter
        pink_b0 = 0.99765f * pink_b0 + w * 0.0990460f;
        pink_b1 = 0.96300f * pink_b1 + w * 0.2965164f;
        pink_b2 = 0.57000f * pink_b2 + w * 1.0526913f;
        noise = pink_b0 + pink_b1 + pink_b2 + w * 0.1848f;
        break;

      case BROWN:
        brown_state = 0.99f * brown_state + 0.01f * w;
        noise = brown_state * 10.0f; // Gain adjust for leaky integrator
        break;

      case BLUE:
        noise = w - 0.5f * last_white;
        break;

      case VIOLET:
        noise = w - last_white;
        break;

      case WHITE:
      default:
        noise = w;
        break;
      }
      last_white = w;

      // Simple click-free gating/smoothing for amp
      amp_ += (target_amp - amp_) * 0.05f;

      // Scale output by 0.15f to match internal logue-sdk headroom
      const float out_val = noise * amp_ * type_gain * 0.15f;

      // Autopan LFO
      autopan_phase_ += lfo_inc;
      if (autopan_phase_ >= 1.0f)
        autopan_phase_ -= 1.0f;

      // osc_sinf takes phase 0-1 and outputs -1 to 1
      float lfo_val = osc_sinf(autopan_phase_);

      // LFO pan oscillates fully between 0 and 1
      float lfo_pan = 0.5f + lfo_val * 0.5f;

      // Interpolate between manual pan and LFO pan
      float current_pan = p.pan * (1.0f - lfo_depth) + lfo_pan * lfo_depth;
      current_pan = clipminmaxf(0.0f, current_pan, 1.0f);

      const float angle = current_pan * 1.57079632679f;
      const float pan_l = osc_sinf(angle * 0.159154943f);
      const float pan_r = osc_cosf(angle * 0.159154943f);

      // Apply panning (out_p[0] is Left, out_p[1] is Right)
      out_p[0] = in_p[0] * dry + out_val * pan_l * wet;
      out_p[1] = in_p[1] * dry + out_val * pan_r * wet;
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    switch (index) {
    case LEVEL:
      params_.level = clipminmaxi32(0, value, 1023) / 1023.f;
      break;
    case PAN:
      params_.pan = clipminmaxi32(0, value, 1023) / 1023.f;
      break;
    case TYPE:
      params_.type = clipminmaxi32(0, value, NUM_TYPES - 1);
      break;
    case DRYWET:
      params_.drywet = clipminmaxi32(0, value, 1023) / 1023.f;
      break;
    case AUTOPAN:
      params_.autopan_depth = clipminmaxi32(0, value, 1023) / 1023.f;
      break;
    case APAN_SPD:
      params_.autopan_speed = clipminmaxi32(0, value, 1023) / 1023.f;
      break;
    case APAN_SYNC:
      params_.autopan_sync = clipminmaxi32(0, value, 6);
      break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
    case LEVEL:
      return (int32_t)(params_.level * 1023.f);
    case PAN:
      return (int32_t)(params_.pan * 1023.f);
    case TYPE:
      return params_.type;
    case DRYWET:
      return (int32_t)(params_.drywet * 1023.f);
    case AUTOPAN:
      return (int32_t)(params_.autopan_depth * 1023.f);
    case APAN_SPD:
      return (int32_t)(params_.autopan_speed * 1023.f);
    case APAN_SYNC:
      return params_.autopan_sync;
    }
    return INT_MIN;
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const {
    if (index == TYPE) {
      static const char *names[] = {"White", "Pink", "Brown", "Blue", "Violet"};
      if (value >= 0 && value < NUM_TYPES)
        return names[value];
    } else if (index == APAN_SYNC) {
      static const char *sync_names[] = {"Off", "1/1",  "1/2", "1/4",
                                         "1/8", "1/16", "1/32"};
      if (value >= 0 && value <= 6)
        return sync_names[value];
    }
    return nullptr;
  }

  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
    (void)id;
    (void)x;
    (void)y;
    if (phase == k_unit_touch_phase_began) {
      is_touched_ = true;
    } else if (phase == k_unit_touch_phase_ended ||
               phase == k_unit_touch_phase_cancelled) {
      is_touched_ = false;
    }
  }

private:
  Params params_;

  // DSP State
  float pink_b0, pink_b1, pink_b2;
  float brown_state;
  float last_white;
  float amp_;
  float bpm_{120.0f};
  float autopan_phase_{0.0f};

  bool is_active_;
  bool is_touched_;
};
