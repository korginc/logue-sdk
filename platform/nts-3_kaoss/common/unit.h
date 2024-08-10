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
 *  @file unit.h
 *
 *  @brief Unit common header
 *
 */

#ifndef UNIT_H_
#define UNIT_H_

#include <stdint.h>

#include "attributes.h"
#include "macros.h"
#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

int8_t unit_init(const unit_runtime_desc_t *);
void unit_teardown();
void unit_reset();
void unit_resume();
void unit_suspend();
void unit_render(const float *, float *, uint32_t);
int32_t unit_get_param_value(uint8_t);
const char * unit_get_param_str_value(uint8_t, int32_t);
void unit_set_param_value(uint8_t, int32_t);
void unit_set_tempo(uint32_t);
void unit_tempo_4ppqn_tick(uint32_t);
void unit_touch_event(uint8_t,uint8_t,uint32_t,uint32_t);
  
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // UNIT_H_
