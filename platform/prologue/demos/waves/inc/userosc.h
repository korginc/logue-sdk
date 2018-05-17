/*
    BSD 3-Clause License

    Copyright (c) 2018, KORG INC.
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
 * @file    userosc.h
 * @brief   C interface header for user oscillators.
 *
 * @addtogroup api
 * @{
 */

#ifndef __userosc_h
#define __userosc_h

#include <stdint.h>

#include "osc_api.h"
#include "userprg.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct user_osc_param {
    int32_t  shape_lfo;
    uint16_t pitch;	// 0x0000 ~ 0x9000?
    uint16_t cutoff;	// 0x0000 ~ 0x1fff
    uint16_t resonance;	// 0x0000 ~ 0x1fff	
    uint16_t padding[3];
  } user_osc_param_t;

  //TODO: would be much better to have shape/shiftshape first and then macros so that we can add more later
  typedef enum {
    k_osc_param_id1 = 0,
    k_osc_param_id2,
    k_osc_param_id3,
    k_osc_param_id4,
    k_osc_param_id5,
    k_osc_param_id6,
    k_osc_param_shape,
    k_osc_param_shiftshape,
  } user_osc_param_id_t;

#define param_val_to_f32(val) ((uint16_t)val * 9.77517106549365e-004f)

  typedef void (*UserOscFuncInit)(uint32_t platform, uint32_t api);
  typedef void (*UserOscFuncCycle)(const user_osc_param_t * const params, int32_t *buf, const uint32_t frames);
  typedef void (*UserOscFuncOn)(const user_osc_param_t * const params);
  typedef void (*UserOscFuncOff)(const user_osc_param_t * const params);
  typedef void (*UserOscFuncMute)(const user_osc_param_t * const params);
  typedef void (*UserOscFuncValue)(uint16_t value);
  typedef void (*UserOscFuncParam)(uint16_t idx, uint16_t value); //TODO: change to 8bit + 32bit

#pragma pack(push, 1)
  typedef struct user_osc_hook_table {
    uint8_t          magic[4];
    uint8_t          padding[12];
    UserOscFuncInit  func_init;
    UserOscFuncCycle func_cycle;
    UserOscFuncOn    func_on;
    UserOscFuncOff   func_off;
    UserOscFuncMute  func_mute;
    UserOscFuncValue func_value;
    UserOscFuncParam func_param;
  } user_osc_hook_table_t;
#pragma pack(pop)

#pragma pack(push, 1)
  typedef struct user_osc_data {
    user_prg_header_t     header;
    user_osc_hook_table_t hooks;
    // ... more bytes following
  } user_osc_data_t;
#pragma pack(pop)

#define OSC_INIT    __attribute__((used)) _hook_init 
#define OSC_CYCLE   __attribute__((used)) _hook_cycle
#define OSC_NOTEON  __attribute__((used)) _hook_on
#define OSC_NOTEOFF __attribute__((used)) _hook_off
#define OSC_MUTE    __attribute__((used)) _hook_mute
#define OSC_VALUE   __attribute__((used)) _hook_value
#define OSC_PARAM   __attribute__((used)) _hook_param

  void _hook_init(uint32_t platform, uint32_t api);
  void _hook_cycle(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames);
  void _hook_on(const user_osc_param_t * const params);
  void _hook_off(const user_osc_param_t * const params);
  void _hook_mute(const user_osc_param_t * const params);
  void _hook_value(uint16_t value);
  void _hook_param(uint16_t index, uint16_t value);
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __userosc_h

/** @} */
