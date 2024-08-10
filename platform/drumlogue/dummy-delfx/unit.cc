/**
 *  @file unit.cc
 *  @brief drumlogue SDK unit interface
 *
 *  Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include "unit.h"   // Note: Include common definitions for all units
#include "delay.h"  // Note: Include custom delay code

static Delay s_delay_instance;              // Note: In this case, actual instance of custom delay object.
static unit_runtime_desc_t s_runtime_desc;  // Note: used to cache runtime descriptor obtained via init callback

// ---- Callback entry points from drumlogue runtime ----------------------------------------------

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
  if (!desc)
    return k_unit_err_undef;

  // Note: make sure the unit is being loaded to the correct platform/module target
  if (desc->target != unit_header.target)
    return k_unit_err_target;

  // Note: check API compatibility with the one this unit was built against
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  // Note: (optional) caching runtime descriptor for future reference
  s_runtime_desc = *desc;

  return s_delay_instance.Init(desc);
}

__unit_callback void unit_teardown() {
  s_delay_instance.Teardown();
}

__unit_callback void unit_reset() {
  s_delay_instance.Reset();
}

__unit_callback void unit_resume() {
  s_delay_instance.Resume();
}

__unit_callback void unit_suspend() {
  s_delay_instance.Suspend();
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  s_delay_instance.Process(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_delay_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_delay_instance.getParameterValue(id);
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_delay_instance.getParameterStrValue(id, value);
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  return s_delay_instance.getParameterBmpValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  // const float t = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
  (void)tempo;
}

__unit_callback void unit_load_preset(uint8_t idx) {
  s_delay_instance.LoadPreset(idx);
}

__unit_callback uint8_t unit_get_preset_index() {
  return s_delay_instance.getPresetIndex();
}

__unit_callback const char * unit_get_preset_name(uint8_t idx) {
  return Delay::getPresetName(idx);
}
