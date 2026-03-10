/**
 * @file unit.cc
 * @brief drumlogue SDK unit interface for OmniPress
 */

#include <cstddef>
#include <cstdint>
#include "unit.h"
#include "masterfx.h"

static MasterFX s_fx_instance;
static unit_runtime_desc_t s_runtime_desc;

__unit_callback int8_t unit_init(const unit_runtime_desc_t* desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != unit_header.target) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;

    s_runtime_desc = *desc;
    return s_fx_instance.Init(desc);
}

__unit_callback void unit_teardown() {
    s_fx_instance.Teardown();
}

__unit_callback void unit_reset() {
    s_fx_instance.Reset();
}

__unit_callback void unit_resume() {
    s_fx_instance.Resume();
}

__unit_callback void unit_suspend() {
    s_fx_instance.Suspend();
}

__unit_callback void unit_render(const float* in, float* out, uint32_t frames) {
    s_fx_instance.Process(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
    s_fx_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return s_fx_instance.getParameterValue(id);
}

__unit_callback const char* unit_get_param_str_value(uint8_t id, int32_t value) {
    return s_fx_instance.getParameterStrValue(id, value);
}

__unit_callback const uint8_t* unit_get_param_bmp_value(uint8_t id, int32_t value) {
    return s_fx_instance.getParameterBmpValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) { (void)tempo; }
__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) { (void)note; (void)velocity; }
__unit_callback void unit_note_off(uint8_t note) { (void)note; }
__unit_callback void unit_gate_on(uint8_t velocity) { (void)velocity; }
__unit_callback void unit_gate_off() {}
__unit_callback void unit_all_note_off() {}
__unit_callback void unit_pitch_bend(uint16_t bend) { (void)bend; }
__unit_callback void unit_channel_pressure(uint8_t pressure) { (void)pressure; }
__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch) {
    (void)note; (void)aftertouch;
}

__unit_callback void unit_load_preset(uint8_t idx) {
    s_fx_instance.LoadPreset(idx);
}

__unit_callback uint8_t unit_get_preset_index() {
    return s_fx_instance.getPresetIndex();
}

__unit_callback const char* unit_get_preset_name(uint8_t idx) {
    return MasterFX::getPresetName(idx);
}