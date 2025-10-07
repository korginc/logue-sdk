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

#include "unit_osc.h"       // Note: Include base definitions for osc units
#include "osc.h"            // Note: Include custom osc code
#include "utils/int_math.h" // clipminmaxi32()

static Osc s_osc_instance;                              // Note: In this example, actual instance of custom osc object.
static int32_t cached_values[UNIT_OSC_MAX_PARAM_COUNT]; // cached parameter values passed from hardware
static const unit_runtime_osc_context_t *context;

// from fixed_math.h
#define q31_to_f32_c 4.65661287307739e-010f
#define q31_to_f32(q) ((float)(q) * q31_to_f32_c)

// ---- Callbacks exposed to runtime ----------------------------------------------
__unit_callback int8_t unit_init(const unit_runtime_desc_t *desc)
{
  if (!desc)
    return k_unit_err_undef;

  // note: make sure the unit is being loaded to the correct platform/module target
  if (desc->target != unit_header.target)
    return k_unit_err_target;

  // note: check API compatibility with the one this unit was built against
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  // check compatibility of samplerate with unit, for NTS-1 MKII should be 48000
  if (desc->samplerate != 48000)
    return k_unit_err_samplerate;

  // Check compatibility of frame geometry
  // note: NTS-1 mkII oscillators can make use of the audio input depending on the routing options in global settings, see product documentation for details.
  if (desc->input_channels != 2 || desc->output_channels != 1) // should be stereo input / mono output
    return k_unit_err_geometry;

  // cache the context for later use
  context = static_cast<const unit_runtime_osc_context_t *>(desc->hooks.runtime_context);

  // initialize cached parameters to defaults
  for (int id = 0; id < UNIT_OSC_MAX_PARAM_COUNT; ++id)
  {
    cached_values[id] = static_cast<int32_t>(unit_header.params[id].init);
  }

  s_osc_instance.init(nullptr);

  return k_unit_err_none;
}

__unit_callback void unit_teardown()
{
  s_osc_instance.teardown();
}

__unit_callback void unit_reset()
{
  s_osc_instance.reset();
}

__unit_callback void unit_resume()
{
  s_osc_instance.resume();
}

__unit_callback void unit_suspend()
{
  s_osc_instance.suspend();
}

__unit_callback void unit_render(const float *in, float *out, uint32_t frames)
{
  s_osc_instance.setPitch(osc_w0f_for_note((context->pitch) >> 8, context->pitch & 0xFF));
  s_osc_instance.setShapeLfo(q31_to_f32(context->shape_lfo));
  s_osc_instance.process(in, out, frames, 1);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
  // clip to valid range as defined in header
  value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max);

  // cache value for unit_get_param_value(id)
  cached_values[id] = value;

  s_osc_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id)
{
  // just return the cached value
  return cached_values[id];
}

__unit_callback const char *unit_get_param_str_value(uint8_t id, int32_t value)
{
  value = clipminmaxi32(unit_header.params[id].min, value, unit_header.params[id].max); // just in case
  return s_osc_instance.getParameterStrValue(id, value);
}

__unit_callback void unit_note_on(uint8_t note, uint8_t velo)
{
  s_osc_instance.noteOn(note, velo);
}

__unit_callback void unit_note_off(uint8_t note)
{
  s_osc_instance.noteOff(note);
}

__unit_callback void unit_all_note_off()
{
  s_osc_instance.allNoteOff();
}

#ifdef ALLOW_DEPRECATED_FUNCTIONS_API_2_1_0

__unit_callback void unit_set_tempo(uint32_t tempo)
{
  s_osc_instance.setTempo(tempo);
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter)
{
  s_osc_instance.tempo4ppqnTick(counter);
}

__unit_callback void unit_pitch_bend(uint16_t bend)
{
  s_osc_instance.pitchBend(bend);
}

__unit_callback void unit_channel_pressure(uint8_t press)
{
  s_osc_instance.channelPressure(press);
}

__unit_callback void unit_aftertouch(uint8_t note, uint8_t press)
{
  s_osc_instance.aftertouch(note, press);
}
#endif