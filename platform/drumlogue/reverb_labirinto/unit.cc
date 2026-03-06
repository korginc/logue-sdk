/**
 * @file unit.cc
 * @brief drumlogue SDK unit interface for Labirinto Reverb
 */

#include <cstddef>
#include <cstdint>
#include "unit.h"    // Note: Include common definitions for all units
#include "NeonAdvancedLabirinto.h"

static NeonAdvancedLabirinto* s_reverb = nullptr;
static unit_runtime_desc_t s_runtime_desc;
static bool s_initialized = false;

__unit_callback int8_t unit_init(const unit_runtime_desc_t* desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != unit_header.target) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;

    s_runtime_desc = *desc;

    // Create reverb instance
    s_reverb = new NeonAdvancedLabirinto();
    if (!s_reverb) {
        return k_unit_err_memory;
    }

    // Initialize with memory allocation
    if (!s_reverb->init()) {
        delete s_reverb;
        s_reverb = nullptr;
        return k_unit_err_memory;
    }

    s_initialized = true;
    return k_unit_err_none;
}

__unit_callback void unit_teardown() {
    if (s_reverb) {
        delete s_reverb;
        s_reverb = nullptr;
    }
    s_initialized = false;
}


__unit_callback void unit_reset() {
  s_reverb.Reset();
}

__unit_callback void unit_resume() {
  s_reverb.Resume();
}

__unit_callback void unit_suspend() {
  s_reverb.Suspend();
}

__unit_callback void unit_render(const float* in, float* out, uint32_t frames) {
    if (!s_initialized || !s_reverb) {
        // Bypass: copy input to output
        memcpy(out, in, frames * 2 * sizeof(float));
        return;
    }

    // Input format: [L, R] interleaved
    // Output format: [L, R] interleaved
    s_reverb->process(in, in + 1, out, out + 1, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_reverb.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_reverb.getParameterValue(id);
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_reverb.getParameterStrValue(id, value);
}

__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  return s_reverb.getParameterBmpValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  // const float t = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
  (void)tempo;
}

__unit_callback void unit_load_preset(uint8_t idx) {
  s_reverb.LoadPreset(idx);
}

__unit_callback uint8_t unit_get_preset_index() {
  return s_reverb.getPresetIndex();
}

__unit_callback const char * unit_get_preset_name(uint8_t idx) {
  return Reverb::getPresetName(idx);
}
