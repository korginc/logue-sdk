/**
 *  @file unit.cc
 *  @brief drumlogue SDK unit interface
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "../common/unit.h"  // Note: Include common definitions for all units

#include <string.h>
#include <cstdint>

#include "ripplerx.h"  // Note: Include custom master fx code

static RipplerX s_synth_instance;      // Note: In this case, actual instance of custom master fx object.
static unit_runtime_desc_t s_runtime_desc;  // Note: used to cache runtime descriptor obtained via init callback

// ---- Callback entry points from drumlogue runtime ----------------------------------------------

/* Called after unit is loaded. Should be used to perform basic checks on runtime environment, initialize the unit,
and perform any dynamic memory allocations if needed.
`desc` provides a description of the current runtime environment (See [Runtime Descriptor](#runtime-descriptor) for details). */
__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
  if (!desc)
    return k_unit_err_undef;

  if (desc->target != unit_header.target)
    return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  s_runtime_desc = *desc;

  return s_synth_instance.Init(desc);
}
/* Called before unit is unloaded. Should be used to perform cleanup and freeing memory if needed.*/
__unit_callback void unit_teardown() {
    s_synth_instance.Teardown();
}

/* Called when unit must be reset to a neutral state. Active notes must be deactivated, enveloppe generators */
__unit_callback void unit_reset() {
  s_synth_instance.Reset();
}
/* Called just before a unit will start processing again after being suspended.*/
__unit_callback void unit_resume() {
  s_synth_instance.Resume();
}
/* Called when a unit is being suspended. For instance, when the currently active unit is being swapped for a different unit.
Usually followed by a call to `unit_reset()`.*/
__unit_callback void unit_suspend() {
  s_synth_instance.Suspend();
}
/* Audio rendering callback. Synth units should ignore the `in` argument.
Input/output buffer geometry information is provided via the `unit_runtime_desc_t` argument of `unit_init(..)`.*/
__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  (void)in;
  memset(out, 0, frames * 2 * sizeof(float));
  s_synth_instance.Render(out, frames);
}
/*  Called to set the current value of the parameter designated by the specified index.
 Note that for the drumlogue values are stored as 16-bit integers, but to avoid future API changes, they are passed as 32bit integers.
 For additional safety, make sure to bound check values as per the min/max values declared in the header.*/
__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_synth_instance.setParameter(id, value);
}
/* Called to obtain the current value of the parameter designated by the specified index.*/
__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_synth_instance.getParameterValue(id);
}
/* Called to obtain the string representation of the specified value, for a `k_unit_param_type_strings` typed parameter.
 The returned value should point to a nul-terminated 7-bit ASCII C string of maximum X characters.
 It can be safely assumed that the C string pointer will not be cached or reused after `unit_get_param_str_value(..)` is called again,
  and thus the same memory area can be reused across calls (if convenient).*/
__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_synth_instance.getParameterStrValue(id, value);
}
/* Called to obtain the bitmap representation of the specified value, for a `k_unit_param_type_bitmaps` typed parameter.
It can be safely assumed that the pointer will not be cached or reused after `unit_get_param_bmp_value(..)` is called again,
and thus the same memory area can be reused across calls (if convenient).
 For details concerning bitmap data format see [Bitmaps](#bitmaps).*/
__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  return s_synth_instance.getParameterBmpValue(id, value);
}
/* Called when a tempo change occurs. The tempo is formatted in fixed point format, with the BPM integer part in the upper 16 bits,
 and fractional part in the lower 16 bits (low endian).
 Care should be taken to keep CPU load as low as possible when handling tempo changes as this handler may be called frequently especially
 if externally synced.*/
__unit_callback void unit_set_tempo(uint32_t tempo) {
  // const float t = (tempo >> 16) + (tempo & 0xFFFF) /
  // static_cast<float>(0x10000);
  (void)tempo;
}
/*  Called upon MIDI note on events, and upon internal sequencer gate on events if an explicit `unit_gate_on(..)` handler is not provided, in which case note will be set to 0xFFU. `velocity` is a 7-bit value.*/
/* FOR PORTING, same as RipplerXAudioProcessor::onNote(MIDIMsg msg)*/
__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) {
  s_synth_instance.NoteOn(note, velocity);
}
/*  Called upon MIDI note off events, and upon internal sequencer gate off events if an explicit `unit_gate_off(..)` handler is not provided, in which case note will be set to 0xFFU.*/
__unit_callback void unit_note_off(uint8_t note) {
  s_synth_instance.NoteOff(note);
}
/* If provided, will be called upon internal sequencer gate on events. `velocity` is a 7-bit value.*/
__unit_callback void unit_gate_on(uint8_t velocity) {
  s_synth_instance.GateOn(velocity);
}
/*  If provided, will be called upon internal sequencer gate off events.*/
__unit_callback void unit_gate_off() {
  s_synth_instance.GateOff();
}
/*  When called all active notes should be deactivated and enveloppe generators reset.*/
__unit_callback void unit_all_note_off() {
  s_synth_instance.AllNoteOff();
}
/*  Called upon MIDI pitch bend event. `bend` is a 14-bit value with neutral center at 0x2000U. Sensitivity can be defined according to the unit's needs.*/
__unit_callback void unit_pitch_bend(uint16_t bend) {
  s_synth_instance.PitchBend(bend);
}
/*  Called upon MIDI channel pressure events. `pressure` is a 7-bit value.*/
__unit_callback void unit_channel_pressure(uint8_t pressure) {
  s_synth_instance.ChannelPressure(pressure);
}
/*  Called upon MIDI aftertouch events. `afterotuch` is a 7-bit value.*/
__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch) {
  s_synth_instance.Aftertouch(note, aftertouch);
}
/* Called to load the preset designated by the specified index. See [Presets](#presets) for details.*/
__unit_callback void unit_load_preset(uint8_t idx) {
  return s_synth_instance.LoadPreset(idx);
}
/* Should return the preset index currently used by the unit.*/
__unit_callback uint8_t unit_get_preset_index() {
  return s_synth_instance.getPresetIndex();
}
/* Called to obtain the name of given preset index. The returned value should point to a nul-terminated 7-bit ASCII C string of maximum X characters.
 The displayable characters are the same as for the unit name.*/
__unit_callback const char * unit_get_preset_name(uint8_t idx) {
  return s_synth_instance.getPresetName(idx);
}
