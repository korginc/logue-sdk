/*
 *  File: unit.cc
 *
 *  NTS-3 kaoss pad kit noise generator unit interface
 *
 */

#include "unit_genericfx.h"
#include "effect.h"

static NoiseGenerator s_instance;

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
  return s_instance.Init(desc);
}

__unit_callback void unit_teardown() {
  s_instance.Teardown();
}

__unit_callback void unit_reset() {
  s_instance.Reset();
}

__unit_callback void unit_resume() {
  s_instance.Resume();
}

__unit_callback void unit_suspend() {
  s_instance.Suspend();
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  s_instance.Process(in, out, (size_t)frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_instance.getParameterValue(id);
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_instance.getParameterStrValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  (void)tempo;
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
  (void)counter;
}

__unit_callback void unit_touch_event(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
  s_instance.touchEvent(id, phase, x, y);
}
