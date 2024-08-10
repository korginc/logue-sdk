/*
    BSD 3-Clause License

    Copyright (c) 2018-2023, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/**
 *  @file _unit_base.c
 *
 *  @brief Fallback implementations for unit header and callbacks
 *
 */

#include <stddef.h>

#include "unit.h"

// ---- Fallback unit header definition  -----------------------------------------------------------

// Fallback unit header, note that this content is invalid, must be overriden
const __attribute__((weak, section(".unit_header"))) unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0,
    .unit_id = 0x0,
    .version = 0x0,
    .name = "undefined",
    .reserved0 = 0x0,
    .num_params = 0x0,
    .params = {{0}}};

// ---- Fallback implementation for exposed unit API -----------------------------------------------

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

__attribute__((weak)) int32_t unit_get_param_value(uint8_t id) {
  (void)id;
  return 0;
}

__attribute__((weak)) const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  (void)id;
  (void)value;
  return NULL;
}

__attribute__((weak)) void unit_set_param_value(uint8_t id, int32_t value) {
  (void)id;
  (void)value;
}

__attribute__((weak)) void unit_set_tempo(uint32_t tempo) { (void)tempo; }

__attribute__((weak)) void unit_tempo_4ppqn_tick(uint32_t counter) { (void)counter; }

__attribute__((weak)) void unit_note_on(uint8_t note, uint8_t mod) {
  (void)note;
  (void)mod;
}

__attribute__((weak)) void unit_note_off(uint8_t note) { (void)note; }

__attribute__((weak)) void unit_all_note_off(void) {}

__attribute__((weak)) void unit_pitch_bend(uint16_t bend) { (void)bend; }

__attribute__((weak)) void unit_channel_pressure(uint8_t mod) { (void)mod; }

__attribute__((weak)) void unit_aftertouch(uint8_t note, uint8_t mod) {
  (void)note;
  (void)mod;
}
