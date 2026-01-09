/*
 * File: osc_miditohz.h
 *
 * MIDI to Hertz conversion lookup table.
 *
 * Author: Etienne Noreau-Hebert <etienne@korg.co.jp>
 * 
 * 2017-2022 (c) Korg
 *
 */
/**
 * @file    osc_miditohz.h
 * @brief   MIDI to Hertz conversion lookup table.
 *
 * @addtogroup DSP
 * @{
 */

#ifndef __midi_to_hz_lut_h
#define __midi_to_hz_lut_h

#ifdef __cplusplus
extern "C" {
#endif

#define k_midi_to_hz_size      (152)
  
extern const float midi_to_hz_lut_f[k_midi_to_hz_size];

#define k_note_mod_fscale      (0.00392156862745098f)
#define k_note_max_hz          (23679.643054f)

#ifdef __cplusplus
}
#endif 


#endif // __midi_to_hz_lut_h