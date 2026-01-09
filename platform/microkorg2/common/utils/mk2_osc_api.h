/**
 * @file    mk2_osc_api.h
 * @brief   Common operations for oscillators
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_mk2_osc_api Oscillator Operations
 * @{
 *
 */

#ifndef __mk2_osc_api_h
#define __mk2_osc_api_h

#include "float_simd.h"
#include "int_simd.h"
#include "osc_api.h"

  /**
   * Get Hertz value for note
   *
   * @param note Note in [0-151] range.
   * @return     Corresponding Hertz value.
   */
  static fast_inline float32x2_t osc_notehzfx2(uint32x2_t note) 
  {
    uint32_t index[2];
    u32x2_str(index, clipmaxu32x2(note, u32x2_dup(k_midi_to_hz_size-1)));
    return float32x2(midi_to_hz_lut_f[index[0]], midi_to_hz_lut_f[index[1]]);
  }

  static fast_inline float32x4_t osc_notehzfx4(uint32x4_t note) 
  {
    uint32_t index[4];
    u32x4_str(index, clipmaxu32x4(note, u32x4_dup(k_midi_to_hz_size-1)));
    return float32x4(midi_to_hz_lut_f[index[0]], 
                     midi_to_hz_lut_f[index[1]],
                     midi_to_hz_lut_f[index[2]],
                     midi_to_hz_lut_f[index[3]]);
  }

  /**
   * Get floating point phase increment for given note and fine modulation
   *
   * @param note Note in [0-151] range, mod in [0-255] range.
   * @return     Corresponding 0-1 phase increment in floating point.
   */
  static fast_inline float32x2_t osc_w0f_for_notex2(uint32x2_t note, float32x2_t mod) 
  {    
    const float32x2_t f0 = osc_notehzfx2(note);
    const float32x2_t f1 = osc_notehzfx2(uint32x2_add(note, u32x2_dup(1)));
    const float32x2_t f = clipmaxfx2(linintfx2(mod, f0, f1), f32x2_dup(k_note_max_hz));
    return float32x2_mulscal(f, k_samplerate_recipf);
  }

  static fast_inline float32x4_t osc_w0f_for_notex4(uint32x4_t note, float32x4_t mod) 
  {    
    const float32x4_t f0 = osc_notehzfx4(note);
    const float32x4_t f1 = osc_notehzfx4(uint32x4_add(note, u32x4_dup(1)));
    const float32x4_t f = clipmaxfx4(linintfx4(mod, f0, f1), f32x4_dup(k_note_max_hz));
    return float32x4_mulscal(f, k_samplerate_recipf);
  }

//** @} */

#endif // __oscillator_api_h

/** @} @} */