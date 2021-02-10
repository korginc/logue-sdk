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
 * @file    userdelfx.h
 * @brief   C interface header for user delay fx.
 *
 * @addtogroup fx Effects
 * @{
 *
 * @addtogroup delfx_inst Delay Effect Instance (delfx)
 * @{
 */

#ifndef __userdelfx_h
#define __userdelfx_h

#include <stdint.h>

#include "fx_api.h"
#include "userprg.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * This macro can be used to declare a memory buffer in SDRAM space.
   *
   * E.g.: float g_my_buffer[1024] __sdram;
   */
#define __sdram __attribute__((section(".sdram")))

  /**
   * User facing parameters
   */
  typedef enum {
    /** Time parameter */
    k_user_delfx_param_time = 0,
    /** Depth parameter */
    k_user_delfx_param_depth,
    k_user_delfx_param_reserved0,
    /** Alternative depth parameter, usually accessible via shift function */
    k_user_delfx_param_shift_depth,
    k_num_user_delfx_param_id
  } user_delfx_param_id_t;

  /** @private */
  typedef void (*UserDelFXFuncEntry)(uint32_t platform, uint32_t api);
  /** @private */
  typedef void (*UserDelFXFuncInit)(uint32_t platform, uint32_t api);
  /** @private */
  typedef void (*UserDelFXFuncProcess)(float *xn, uint32_t frames);
  /** @private */
  typedef void (*UserDelFXSuspend)(void);
  /** @private */
  typedef void (*UserDelFXResume)(void);
  /** @private */
  typedef void (*UserDelFXFuncParam)(uint8_t index, int32_t value);
  /** @private */
  typedef void (*UserDelFXFuncDummy)(void);

  /** @private */
#pragma pack(push, 1)
  typedef struct user_delfx_hook_table {
    uint8_t              magic[4];
    uint32_t             api;
    uint8_t              platform;
    uint8_t              reserved0[7];
    
    UserDelFXFuncEntry   func_entry;
    UserDelFXFuncProcess func_process;
    UserDelFXSuspend     func_suspend;
    UserDelFXResume      func_resume;
    UserDelFXFuncParam   func_param;
    UserDelFXFuncDummy   reserved1[7];
  } user_delfx_hook_table_t;
#pragma pack(pop)

  /** @private */
#pragma pack(push, 1)
  typedef struct user_delfx_data {
    user_prg_header_t       header;
    user_delfx_hook_table_t hooks;
    // code bytes following
  } user_delfx_data_t;
#pragma pack(pop)

  /**
   * @name    Core API
   * @{
   */

#define DELFX_INIT    __attribute__((used)) _hook_init 
#define DELFX_PROCESS __attribute__((used)) _hook_process
#define DELFX_SUSPEND __attribute__((used)) _hook_suspend
#define DELFX_RESUME  __attribute__((used)) _hook_resume
#define DELFX_PARAM   __attribute__((used)) _hook_param

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
   * Processing callback. Must be implemented by your custom effect.
   *
   * Delay effects must write back their result to the input buffer and are responsible for wet/dry mix balance.
   * Typically wet/dry balance should be controllable via the shift-depth parameter.
   *
   * @param xn Input/output sample buffer. 
   * @param frames Size of buffer in frames. (2 samples per frames)
   *
   * @note Implementation must support at least up to 64 frames.
   * @note Optimize for powers of two.
   */  
  void _hook_process(float *xn, uint32_t frames);

  /**
   * Suspend callback. Must be implemented by your custom effect.
   *
   * Called before effect gets suspended. While suspended the processing callback will not be called.
   */    
  void _hook_suspend(void);

  /**
   * Resume callback. Must be implemented by your custom effect.
   *
   * Called before calls to processing callback resume after being suspended.
   */    
  void _hook_resume(void);

  /**
   * Parameter change callback. Must be implemented by your custom effect.
   *
   * @param index Parameter ID. See user_delfx_param_id_t.
   * @param index Parameter value. 
   *
   * @note All parameters have 10 bit resolution.
   */
  void _hook_param(uint8_t index, int32_t value);

  /** @} */
  
#ifdef __cplusplus
} // extern "C"
#endif
  
#endif // __userdelfx_h

/** @} @} */
