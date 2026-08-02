/*
 * twosaw.cpp — a small gen-1 oscillator: two detuned sawtooths.
 *
 * A worked, self-contained example used to demonstrate the websim gen-1 osc
 * harness with a non-`waves` unit whose slider metadata comes from manifest.json
 * (see WEBSIM.md §C.2 / WEBSIM_FOLLOWUP_PLAN.md §C.3). It needs no firmware ROM
 * helpers — phase accumulators + naive saws only — so it builds for both the
 * hardware and the sim. Two custom params (Detune, Mix) plus the SHAPE knob.
 */
#include "userosc.h"

#include <cmath>

namespace {
constexpr float kSr = 48000.f;

float s_phase1 = 0.f, s_phase2 = 0.f;
float s_detune = 0.f;  // 0..1  -> spread between the two saws
float s_mix = 0.f;     // 0..1  -> amount of saw2 vs saw1
float s_shape = 0.f;   // SHAPE knob 0..1 -> extra detune

inline float pitch_to_w0(uint16_t pitch) {
  const float notef = (float)(pitch >> 8) + (float)(pitch & 0xFF) * (1.f / 256.f);
  const float hz = 440.f * exp2f((notef - 69.f) * (1.f / 12.f));
  return hz / kSr;  // cycles per sample
}

inline float wrap1(float p) { return p - (float)(int)p; }
}  // namespace

void OSC_INIT(uint32_t platform, uint32_t api) {
  (void)platform;
  (void)api;
  s_phase1 = s_phase2 = 0.f;
}

void OSC_CYCLE(const user_osc_param_t *const params, int32_t *yn, const uint32_t frames) {
  const float w0 = pitch_to_w0(params->pitch);
  const float cents = (s_detune + s_shape) * 50.f;  // up to ~100 cents of spread
  const float ratio = exp2f(cents * (1.f / 1200.f));
  const float w1 = w0 / ratio;  // saw 1 a touch flat
  const float w2 = w0 * ratio;  // saw 2 a touch sharp
  const float g2 = s_mix;
  const float g1 = 1.f - s_mix;

  for (uint32_t i = 0; i < frames; i++) {
    s_phase1 = wrap1(s_phase1 + w1);
    s_phase2 = wrap1(s_phase2 + w2);
    const float saw1 = 2.f * s_phase1 - 1.f;
    const float saw2 = 2.f * s_phase2 - 1.f;
    float sig = 0.5f * (g1 * saw1 + g2 * saw2);
    if (sig > 1.f) sig = 1.f;
    else if (sig < -1.f) sig = -1.f;
    yn[i] = (int32_t)(sig * 2147483647.f);  // float -> q31
  }
}

void OSC_NOTEON(const user_osc_param_t *const params) { (void)params; }
void OSC_NOTEOFF(const user_osc_param_t *const params) { (void)params; }

void OSC_PARAM(uint16_t index, uint16_t value) {
  switch (index) {
    case k_user_osc_param_id1:  // "Detune", manifest 0..100
      s_detune = value * (1.f / 100.f);
      break;
    case k_user_osc_param_id2:  // "Mix", manifest 0..100
      s_mix = value * (1.f / 100.f);
      break;
    case k_user_osc_param_shape:  // 10-bit knob
      s_shape = value * (1.f / 1023.f);
      break;
    default:
      break;
  }
}
