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
 * @file    usermodfx.h
 * @brief   C interface header for user modulation fx.
 *
 * @addtogroup api
 * @{
 */

#ifndef __usermodfx_h
#define __usermodfx_h

#include <stdint.h>

#include "fx_api.h"
#include "userprg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define __sdram __attribute__((section(".sdram")))

  typedef void (*UserModFXFuncInit)(uint32_t platform, uint32_t api);
  typedef void (*UserModFXFuncProcess)(const float *main_xn, float *main_yn,
                                       const float *sub_xn,  float *sub_yn,
                                       uint32_t frames);
  typedef void (*UserModFXSuspend)(void);
  typedef void (*UserModFXResume)(void);
  typedef void (*UserModFXFuncParam)(uint8_t index, int32_t value);
  
#pragma pack(push, 1)
  typedef struct user_modfx_hook_table {
    uint8_t              magic[4];
    uint8_t              padding[12];
    UserModFXFuncInit    func_init;
    UserModFXFuncProcess func_process;
    UserModFXSuspend     func_suspend;
    UserModFXResume      func_resume;
    UserModFXFuncParam   func_param;
  } user_modfx_hook_table_t;
#pragma pack(pop)
  
#pragma pack(push, 1)
  typedef struct user_modfx_data {
    user_prg_header_t       header;
    user_modfx_hook_table_t hooks;
    // code bytes following
    // user_prg_sig_t data located load_size bytes after header
  } user_modfx_data_t;
#pragma pack(pop)

#define MODFX_INIT    __attribute__((used)) _hook_init 
#define MODFX_PROCESS __attribute__((used)) _hook_process
#define MODFX_SUSPEND __attribute__((used)) _hook_suspend
#define MODFX_RESUME  __attribute__((used)) _hook_resume
#define MODFX_PARAM   __attribute__((used)) _hook_param

  void _hook_init(uint32_t platform, uint32_t api);
  void _hook_process(const float *main_xn, float *main_yn,
                     const float *sub_xn, float *sub_yn,
                     uint32_t frames);
  void _hook_suspend(void);
  void _hook_resume(void);
  void _hook_param(uint8_t index, int32_t value);
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __usermodfx_h

