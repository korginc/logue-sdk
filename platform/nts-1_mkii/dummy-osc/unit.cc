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
 *  NTS-1 mkII oscillator unit interface
 *
 */

#include "unit_osc.h"                       // Note: Include base definitions for osc units

#include "osc.h"                            // Note: Include custom osc code

static Osc s_osc_instance;                  // Note: In this example, actual instance of custom osc object.

// ---- Callbacks exposed to runtime ----------------------------------------------

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
  return s_osc_instance.Init(desc);
}

__unit_callback void unit_teardown() {
  s_osc_instance.Teardown();
}

__unit_callback void unit_reset() {
  s_osc_instance.Reset();
}

__unit_callback void unit_resume() {
  s_osc_instance.Resume();
}

__unit_callback void unit_suspend() {
  s_osc_instance.Suspend();
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  s_osc_instance.Process(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value) {
  s_osc_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
  return s_osc_instance.getParameterValue(id);
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value) {
  return s_osc_instance.getParameterStrValue(id, value);
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  s_osc_instance.setTempo(tempo);
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
  s_osc_instance.tempo4ppqnTick(counter);
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velo) {
  s_osc_instance.NoteOn(note, velo);
}

__unit_callback void unit_note_off(uint8_t note) {
  s_osc_instance.NoteOff(note);
}

__unit_callback void unit_all_note_off() {
  s_osc_instance.AllNoteOff();
}

__unit_callback void unit_pitch_bend(uint16_t bend) {
  s_osc_instance.PitchBend(bend);
}

__unit_callback void unit_channel_pressure(uint8_t press) {
  s_osc_instance.ChannelPressure(press);
}

__unit_callback void unit_aftertouch(uint8_t note, uint8_t press) {
  s_osc_instance.AfterTouch(note, press);
}
