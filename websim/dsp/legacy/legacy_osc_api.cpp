/*
 * websim host implementation of the gen-1 (legacy) oscillator ROM functions.
 *
 * The gen-1 osc_api.h defines osc_white()/osc_rand()/osc_mcu_hash()/osc_bl_*_idx()
 * as inline forwarders to underscore-prefixed ROM symbols (_osc_white, etc.).
 * On hardware those live in firmware ROM; here we provide host equivalents,
 * analogous to the gen-2 websim/dsp/osc_api.cpp. The wavetable note tables
 * (wt_*_notes) come from the shared websim/dsp/wavetable_lut.c. See
 * WEBSIM_EXPANSION_PLAN.md §6.2.
 */

#include <climits>
#include <stdint.h>

#include "osc_api.h" // gen-1: osc_sinf, osc_sqrtm2logf, wt_*_notes externs, clampmaxfsel

namespace {

// Park-Miller-Carta gaussian white-noise source (matches the ROM behaviour).
struct NoiseFlt {
  uint32_t mState = 83647U;

  inline uint32_t rand(uint32_t state) {
    uint32_t lo = 16807u * (state & 0xFFFF);
    const uint32_t hi = 16807u * (state >> 16);
    lo += (hi & 0x7FFF) << 16;
    lo += hi >> 15;
    lo = (lo & 0x7FFFFFFF) + (lo >> 31);
    return lo;
  }

  inline float white(void) {
    static const float scale = 1.f / (float)UINT_MAX;
    const uint32_t r1 = rand(mState);
    const uint32_t r2 = rand(r1);
    mState = r2;
    const float r1f = r1 * scale;
    const float r2f = r2 * scale;
    return 0.3f * (osc_sqrtm2logf(r1f) * osc_sinf(r2f + 0.25f));
  }
};

NoiseFlt s_noise;
uint32_t s_mcu_hash = 0;

inline float wt_note_offset_frac(const float note, const uint8_t *notes, const uint8_t len) {
  const uint8_t *n = notes;
  const uint8_t *n_e = n + (len - 1);
  uint8_t i = 0;
  for (; n != n_e; ++n, ++i)
    if ((*n) >= note)
      break;
  const uint8_t prev = (i > 0) ? notes[i - 1] : 0;
  return clampmaxfsel(i + (float)(note - prev) / (notes[i] - prev), len - 1);
}

} // namespace

extern "C" {

uint32_t _osc_mcu_hash(void) { return s_mcu_hash; }

uint32_t _osc_rand(void) {
  const uint32_t r = s_noise.rand(s_noise.mState);
  s_noise.mState = r;
  return r;
}

float _osc_white(void) { return s_noise.white(); }

float _osc_bl_saw_idx(float note) { return wt_note_offset_frac(note, wt_saw_notes, k_wt_saw_notes_cnt); }
float _osc_bl_sqr_idx(float note) { return wt_note_offset_frac(note, wt_sqr_notes, k_wt_sqr_notes_cnt); }
float _osc_bl_par_idx(float note) { return wt_note_offset_frac(note, wt_par_notes, k_wt_par_notes_cnt); }

} // extern "C"
