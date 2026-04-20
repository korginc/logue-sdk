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
  enum { LEVEL = 0U, PAN, TYPE, NUM_PARAMS };

  enum NoiseType { WHITE = 0, PINK, BROWN, BLUE, VIOLET, NUM_TYPES };

  struct Params {
    float level{0.5f};
    float pan{0.5f};
    uint8_t type{WHITE};

    void reset() {
      level = 0.5f;
      pan = 0.5f;
      type = WHITE;
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

  inline void reset_state() {
    pink_b0 = pink_b1 = pink_b2 = 0.f;
    brown_state = 0.f;
    last_white = 0.f;
    amp_ = 0.f;
    is_active_ = false;
    is_touched_ = false;
  }

  fast_inline void Process(const float *in, float *out, size_t frames) {
    const float *__restrict in_p = in;
    float *__restrict out_p = out;
    const float *out_e = out_p + (frames << 1);

    const Params p = params_;

    // Constant-power panning coefficients
    // Pan 0.0 -> Left, 0.5 -> Center, 1.0 -> Right
    // angle = pan * PI/2
    const float angle = p.pan * 1.57079632679f;
    const float pan_l = osc_cosf(
        angle * 0.159154943f); // osc_cosf expects 0-1 range (angle/2PI)
    const float pan_r = osc_sinf(angle * 0.159154943f);

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

      float out_val = noise * amp_ * type_gain;

      out_p[0] = out_val * pan_r;
      out_p[1] = out_val * pan_l;
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
    }
    return INT_MIN;
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const {
    if (index == TYPE) {
      static const char *names[] = {"White", "Pink", "Brown", "Blue", "Violet"};
      if (value >= 0 && value < NUM_TYPES)
        return names[value];
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

  bool is_active_;
  bool is_touched_;
};
