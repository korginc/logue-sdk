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
 * @addtogroup osc Oscillator
 * @{
 *
 * @addtogroup osc_inst Oscillator Instance (osc)
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

  /**
   * Internal realtime parameters
   */
  typedef struct user_osc_param {
    /** Value of LFO implicitely applied to shape parameter */
    int32_t  shape_lfo;
    /** Current pitch. high byte: note number, low byte: fine (0-255) */
    uint16_t pitch;
    /** Current cutoff value (0x0000-0x1fff) */
    uint16_t cutoff;
    /** Current resonance value (0x0000-0x1fff) */
    uint16_t resonance;
    uint16_t reserved0[3];
  } user_osc_param_t;

  /**
   * User facing osc-specific parameters
   */
  typedef enum {
    /** Edit parameter 1 */
    k_user_osc_param_id1 = 0,
    /** Edit parameter 2 */
    k_user_osc_param_id2,
    /** Edit parameter 3 */
    k_user_osc_param_id3,
    /** Edit parameter 4 */
    k_user_osc_param_id4,
    /** Edit parameter 5 */
    k_user_osc_param_id5,
    /** Edit parameter 6 */
    k_user_osc_param_id6,
    /** Shape parameter */
    k_user_osc_param_shape,
    /** Alternative Shape parameter: generally available via a shift function */
    k_user_osc_param_shiftshape,
    k_num_user_osc_param_id
  } user_osc_param_id_t;

  /**
   * Convert 10bit parameter values to float
   */
#define param_val_to_f32(val) ((uint16_t)val * 9.77517106549365e-004f)  
  
  /** @private */
  typedef void (*UserOscFuncEntry)(uint32_t platform, uint32_t api);
  /** @private */
  typedef void (*UserOscFuncInit)(uint32_t platform, uint32_t api);
  /** @private */
  typedef void (*UserOscFuncCycle)(const user_osc_param_t * const params, int32_t *buf, const uint32_t frames);
  /** @private */
  typedef void (*UserOscFuncOn)(const user_osc_param_t * const params);
  /** @private */
  typedef void (*UserOscFuncOff)(const user_osc_param_t * const params);
  /** @private */
  typedef void (*UserOscFuncMute)(const user_osc_param_t * const params);
  /** @private */
  typedef void (*UserOscFuncValue)(uint16_t value);
  /** @private */
  typedef void (*UserOscFuncParam)(uint16_t idx, uint16_t value);
  /** @private */
  typedef void (*UserOscFuncDummy)(void);

  /** @private */
#pragma pack(push, 1)
  typedef struct user_osc_hook_table {
    uint8_t          magic[4];
    uint32_t         api;
    uint8_t          platform;
    uint8_t          reserved0[7];
    
    UserOscFuncInit  func_entry;
    UserOscFuncCycle func_cycle;
    UserOscFuncOn    func_on;
    UserOscFuncOff   func_off;
    UserOscFuncMute  func_mute;
    UserOscFuncValue func_value;
    UserOscFuncParam func_param;
    UserOscFuncDummy reserved1[5];
  } user_osc_hook_table_t;
#pragma pack(pop)

  /** @private */
#pragma pack(push, 1)
  typedef struct user_osc_data {
    user_prg_header_t     header;
    user_osc_hook_table_t hooks;
    // ... more bytes following
  } user_osc_data_t;
#pragma pack(pop)
  
  /**
   * @name    Core API
   * @{
   */
  
#define OSC_INIT    __attribute__((used)) _hook_init 
#define OSC_CYCLE   __attribute__((used)) _hook_cycle
#define OSC_NOTEON  __attribute__((used)) _hook_on
#define OSC_NOTEOFF __attribute__((used)) _hook_off
#define OSC_MUTE    __attribute__((used)) _hook_mute
#define OSC_VALUE   __attribute__((used)) _hook_value
#define OSC_PARAM   __attribute__((used)) _hook_param

  /** @private */
  void _entry(uint32_t platform, uint32_t api);

  /**
   * Initialization callback. Must be implemented by your custom effect.
   *
   * @param platform Current target platform/module. See userprg.h
   * @param api Current API version. See userprg.h
   */
  void _hook_init(uint32_t platform, uint32_t api);

  /**
   * Rendering callback. Must be implemented by your custom effect.
   *
   * @param params Current realtime parameter state.
   * @param yn Output buffer. (1 sample per frame)
   * @param frames Size of output buffer.
   *
   * @note Implementation must support at least up to 64 frames.
   * @note Optimize for powers of two.
   */  
  void _hook_cycle(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames);

  /**
   * Note on callback. Must be implemented by your custom effect.
   *
   * @param params Current realtime parameter state.
   */  
  void _hook_on(const user_osc_param_t * const params);

  /**
   * Note off callback. Must be implemented by your custom effect.
   *
   * @param params Current realtime parameter state.
   */    
  void _hook_off(const user_osc_param_t * const params);

  /** @private */
  void _hook_mute(const user_osc_param_t * const params);

  /** @private */
  void _hook_value(uint16_t value);

  /**
   * Parameter change callback. Must be implemented by your custom effect.
   *
   * @param index Parameter ID. See user_osc_param_id_t.
   * @param index Parameter value. 
   *
   * @note 10 bit resolution for shape/shift-shape. 
   * @note 0-200 for bipolar percent parameters. 0% at 100, -100% at 0.
   * @note 0-100 for unipolar percent and typeless parameters.
   */    
  void _hook_param(uint16_t index, uint16_t value);

  /** @} */
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __userosc_h

/** @} @} */
