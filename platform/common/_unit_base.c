/**
 * @file _unit_base.c
 * @brief Fallback implementations for unit header and callbacks
 *
 * Copyright (c) 2020-2022 KORG Inc. All rights reserved.
 *
 */

#include <stddef.h>

#include "unit.h"

// ---- Fallback unit header definition  -----------------------------------------------------------

// Fallback unit header, note that this content is invalid, must be overridden
const __attribute__((weak, section(".unit_header"))) unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM_INVALID,
    .api = UNIT_API_VERSION_INVALID,
    .dev_id = 0x0,
    .unit_id = 0x0,
    .version = 0x0,
    .name = "undefined",
    .num_presets = 0x0,
    .num_params = 0x0,
    .params = {{0}}};

// ---- Fallback entry points from drumlogue runtime ----------------------------------------------

__attribute__((weak)) int8_t unit_init(const unit_runtime_desc_t * desc) {
  (void)desc;
  return k_unit_err_undef;
}

__attribute__((weak)) void unit_teardown() {}

__attribute__((weak)) void unit_reset() {}

__attribute__((weak)) void unit_resume() {}

__attribute__((weak)) void unit_suspend() {}

__attribute__((weak)) void unit_render(const float * in, float * out, uint32_t frames) {
  (void)in;
  (void)out;
  (void)frames;
}

__attribute__((weak)) uint8_t unit_get_preset_index() {
  return 0;
}

__attribute__((weak)) const char * unit_get_preset_name(uint8_t idx) {
  (void)idx;
  return NULL;
}

__attribute__((weak)) void unit_load_preset(uint8_t idx) {
  (void)idx;
}

__attribute__((weak)) int32_t unit_get_param_value(uint8_t id) {
  (void)id;
  return 0;
}

__attribute__((weak)) const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  (void)id;
  (void)value;
  return NULL;
}

__attribute__((weak)) const uint8_t * unit_get_param_bmp_value(uint8_t id, int32_t value) {
  (void)id;
  (void)value;
  return NULL;
}

__attribute__((weak)) void unit_set_param_value(uint8_t id, int32_t value) {
  (void)id;
  (void)value;
}

__attribute__((weak)) void unit_set_tempo(uint32_t tempo) { (void)tempo; }

__attribute__((weak)) void unit_note_on(uint8_t note, uint8_t mod) {
  (void)note;
  (void)mod;
}

__attribute__((weak)) void unit_note_off(uint8_t note) { (void)note; }

__attribute__((weak)) void unit_gate_on(uint8_t mod) { (void)mod; }

__attribute__((weak)) void unit_gate_off(void) {}

__attribute__((weak)) void unit_all_note_off(void) {}

__attribute__((weak)) void unit_pitch_bend(uint16_t bend) { (void)bend; }

__attribute__((weak)) void unit_channel_pressure(uint8_t mod) { (void)mod; }

__attribute__((weak)) void unit_aftertouch(uint8_t note, uint8_t mod) {
  (void)note;
  (void)mod;
}

__attribute__((weak)) void unit_platform_exclusive(uint8_t messageId, void * data, uint32_t dataSize) {
  (void)messageId;
  (void)data;
  (void)dataSize;
}