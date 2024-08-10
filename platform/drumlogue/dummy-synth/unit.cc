/**
 *  @file unit.cc
 *  @brief drumlogue SDK unit interface
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include <cstddef>
#include <cstdint>

#include "unit.h"   // Note: Include common definitions for all units
#include "synth.h"  // Note: Include custom master fx code

static Synth s_synth_instance;              // Note: In this case, actual instance of custom master fx object.
static unit_runtime_desc_t s_runtime_desc;  // Note: used to cache runtime descriptor obtained via init callback

// ---- Callback entry points from drumlogue runtime ----------------------------------------------

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

__unit_callback void unit_teardown() {
  s_synth_instance.Teardown();
}

__unit_callback void unit_reset() {
  s_synth_instance.Reset();
}

__unit_callback void unit_resume() {
  s_synth_instance.Resume();
}

__unit_callback void unit_suspend() {
  s_synth_instance.Suspend();
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  (void)in;
  s_synth_instance.Render(out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_synth_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_synth_instance.getParameterValue(id);
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_synth_instance.getParameterStrValue(id, value);
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  return s_synth_instance.getParameterBmpValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  // const float t = (tempo >> 16) + (tempo & 0xFFFF) /
  // static_cast<float>(0x10000);
  (void)tempo;
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) {
  s_synth_instance.NoteOn(note, velocity);
}

__unit_callback void unit_note_off(uint8_t note) {
  s_synth_instance.NoteOff(note);
}

__unit_callback void unit_gate_on(uint8_t velocity) {
  s_synth_instance.GateOn(velocity);
}

__unit_callback void unit_gate_off() {
  s_synth_instance.GateOff();
}

__unit_callback void unit_all_note_off() {
  s_synth_instance.AllNoteOff();
}

__unit_callback void unit_pitch_bend(uint16_t bend) {
  s_synth_instance.PitchBend(bend);
}

__unit_callback void unit_channel_pressure(uint8_t pressure) {
  s_synth_instance.ChannelPressure(pressure);
}

__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch) {
  s_synth_instance.Aftertouch(note, aftertouch);
}

__unit_callback void unit_load_preset(uint8_t idx) {
  return s_synth_instance.LoadPreset(idx);
}

__unit_callback uint8_t unit_get_preset_index() {
  return s_synth_instance.getPresetIndex();
}

__unit_callback const char * unit_get_preset_name(uint8_t idx) {
  return Synth::getPresetName(idx);
}
