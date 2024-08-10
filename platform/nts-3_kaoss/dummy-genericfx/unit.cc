/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
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

/*
 *  File: unit.cc
 *
 *  NTS-3 kaoss pad kit generic effect unit interface
 *
 */

#include "unit_genericfx.h"                 // Note: Include base definitions for genericfx units

#include "effect.h"                         // Note: Include template effect code

static Effect s_effect_instance;            // Note: In this example, actual effect instance.

// ---- Callbacks exposed to runtime ----------------------------------------------

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
  return s_effect_instance.Init(desc);
}

__unit_callback void unit_teardown() {
  s_effect_instance.Teardown();
}

__unit_callback void unit_reset() {
  s_effect_instance.Reset();
}

__unit_callback void unit_resume() {
  s_effect_instance.Resume();
}

__unit_callback void unit_suspend() {
  s_effect_instance.Suspend();
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  s_effect_instance.Process(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_effect_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_effect_instance.getParameterValue(id);
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_effect_instance.getParameterStrValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  s_effect_instance.setTempo(tempo);
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
  s_effect_instance.tempo4ppqnTick(counter);
}

__unit_callback void unit_touch_event(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
  s_effect_instance.touchEvent(id, phase, x, y);
}
  
