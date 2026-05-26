/*
 *  File: unit.cc
 *  DeepFM — 2-operator FM oscillator for microKORG 2
 */

#include "unit.h"
#include "runtime.h"
#include <cstddef>
#include <cstdint>
#include "deepfm.h"

static DeepFM s_deepfm_instance;
static unit_runtime_desc_t s_runtime_desc;

__attribute__((used)) int8_t unit_init(const unit_runtime_desc_t * desc) {
  if (!desc)
    return k_unit_err_undef;
  if (desc->target != unit_header.target)
    return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;
  s_runtime_desc = *desc;
  return s_deepfm_instance.Init(desc);
}

__attribute__((used)) void unit_teardown() {
  s_deepfm_instance.Teardown();
}

__attribute__((used)) void unit_reset() {
  s_deepfm_instance.Reset();
}

__attribute__((used)) void unit_resume() {
  s_deepfm_instance.Resume();
}

__attribute__((used)) void unit_suspend() {
  s_deepfm_instance.Suspend();
}

__attribute__((used)) void unit_render(const float * in, float * out, uint32_t frames) {
  (void)in;
  s_deepfm_instance.Process(out, frames);
}

__attribute__((used)) void unit_set_param_value(uint8_t id, int32_t value) {
  s_deepfm_instance.setParameter(id, value);
}

__attribute__((used)) int32_t unit_get_param_value(uint8_t id) {
  return s_deepfm_instance.getParameterValue(id);
}

__attribute__((used)) const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_deepfm_instance.getParameterStrValue(id, value);
}

__attribute__((used)) const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  return s_deepfm_instance.getParameterBmpValue(id, value);
}

__attribute__((used)) void unit_set_tempo(uint32_t tempo) {
  (void)tempo;
}

__attribute__((used)) void unit_load_preset(uint8_t idx) {
  s_deepfm_instance.LoadPreset(idx);
}

__attribute__((used)) uint8_t unit_get_preset_index() {
  return s_deepfm_instance.getPresetIndex();
}

__attribute__((used)) void unit_platform_exclusive(uint8_t messageId, void * data, uint32_t dataSize) {
  s_deepfm_instance.unit_platform_exclusive(messageId, data, dataSize);
}
