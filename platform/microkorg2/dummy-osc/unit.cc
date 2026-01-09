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
 *  Microkorg2 Oscillator unit interface
 *
 */

#include "unit.h"

#include <cstddef>
#include <cstdint>

#include "osc.h"

static Osc s_osc_instance;
static unit_runtime_desc_t s_runtime_desc;

__attribute__((used)) int8_t unit_init(const unit_runtime_desc_t * desc) {
  if (!desc)
    return k_unit_err_undef;

  if (desc->target != unit_header.target)
    return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;

  s_runtime_desc = *desc;

  return s_osc_instance.Init(desc);
}

__attribute__((used)) void unit_teardown() {
  s_osc_instance.Teardown();
}

__attribute__((used)) void unit_reset() {
  s_osc_instance.Reset();
}

__attribute__((used)) void unit_resume() {
  s_osc_instance.Resume();
}

__attribute__((used)) void unit_suspend() {
  s_osc_instance.Suspend();
}

__attribute__((used)) void unit_render(const float * in, float * out, uint32_t frames) {
  (void)in;
  s_osc_instance.Process(in, out, frames);
}

__attribute__((used)) void unit_set_param_value(uint8_t id, int32_t value) {
  s_osc_instance.setParameter(id, value);
}

__attribute__((used)) int32_t unit_get_param_value(uint8_t id) {
  return s_osc_instance.getParameterValue(id);
}

__attribute__((used)) const char * unit_get_param_str_value(uint8_t id,
                                                            int32_t value) {
  return s_osc_instance.getParameterStrValue(id, value);
}
