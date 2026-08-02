/*
 * tremolo.cpp — a minimal gen-1 modfx: amplitude modulation by a sine LFO.
 *
 * Deliberately tiny; it exists to exercise the websim gen-1 fx harness end to
 * end (the MODFX_INIT/PROCESS/PARAM entry points through legacy_fx_bridge.h).
 * Builds for hardware too (ordinary gen-1 modfx).
 */
#include "usermodfx.h"

#include <cmath>

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kSr = 48000.f;

float s_phase = 0.f;
float s_rate = 0.5f;   // 0..1 -> 0.1 .. 10 Hz
float s_depth = 0.5f;  // 0..1
}  // namespace

void MODFX_INIT(uint32_t platform, uint32_t api) {
  (void)platform;
  (void)api;
  s_phase = 0.f;
}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn,
                   float *sub_yn, uint32_t frames) {
  (void)sub_xn;
  (void)sub_yn;
  const float rate_hz = 0.1f + s_rate * 9.9f;
  const float inc = kTwoPi * rate_hz / kSr;
  for (uint32_t i = 0; i < frames; ++i) {
    const float lfo = 0.5f * (1.f + sinf(s_phase));  // 0..1
    const float g = 1.f - s_depth * lfo;             // depth-controlled dip
    main_yn[2 * i] = main_xn[2 * i] * g;
    main_yn[2 * i + 1] = main_xn[2 * i + 1] * g;
    s_phase += inc;
    if (s_phase >= kTwoPi) s_phase -= kTwoPi;
  }
}

void MODFX_PARAM(uint8_t index, int32_t value) {
  // websim drives this with the 0..1023 slider value; hardware passes a similar
  // 10-bit range for the TIME / DEPTH knobs.
  const float v = value * (1.f / 1023.f);
  switch (index) {
    case k_user_modfx_param_time:
      s_rate = v;
      break;
    case k_user_modfx_param_depth:
      s_depth = v;
      break;
    default:
      break;
  }
}
