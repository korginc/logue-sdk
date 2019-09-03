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
 * @file    _unit.c
 * @brief   Modulation effect entry template.
 *
 * @addtogroup api
 * @{
 */

#include "usermodfx.h"

/*===========================================================================*/
/* Externs and Types.                                                        */
/*===========================================================================*/

/**
 * @name   Externs and Types.
 * @{
 */

extern uint8_t _bss_start;
extern uint8_t _bss_end;

extern void (*__init_array_start []) (void);
extern void (*__init_array_end []) (void);

typedef void (*__init_fptr)(void);

/** @} */

/*===========================================================================*/
/* Local Constants and Vars.                                                 */
/*===========================================================================*/

/**
 * @name   Local Constants and Vars.
 * @{
 */

__attribute__((used, section(".hooks")))
static const user_modfx_hook_table_t s_hook_table = {
  .magic = {'U','M','O','D'},
  .api = USER_API_VERSION,
  .platform = USER_TARGET_PLATFORM>>8,
  .reserved0 = {0},
  .func_entry = _entry,
  .func_process = _hook_process,
  .func_suspend = _hook_suspend,
  .func_resume = _hook_resume,
  .func_param = _hook_param,
  .reserved1 = {0}
};

/** @} */

/*===========================================================================*/
/* Default Hooks.                                                             */
/*===========================================================================*/

/**
 * @name   Default Hooks.
 * @{
 */

__attribute__((used))
void _entry(uint32_t platform, uint32_t api)
{
  // Ensure zero-clear BSS segment
  uint8_t * __restrict bss_p = (uint8_t *)&_bss_start;
  const uint8_t * const bss_e = (uint8_t *)&_bss_end;

  for (; bss_p != bss_e;)
    *(bss_p++) = 0;

  // Call constructors if any.  
  const size_t count = __init_array_end - __init_array_start;
  for (size_t i = 0; i<count; ++i) {
    __init_fptr init_p = (__init_fptr)__init_array_start[i];
    if (init_p != NULL)
      init_p();
  }
  
  // Call user initialization
  _hook_init(platform, api);
}

__attribute__((weak))
void _hook_init(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
}

__attribute__((weak))
void _hook_process(const float *main_xn, float *main_yn,
                   const float *sub_xn, float *sub_yn,
                   uint32_t frames)
{
  (void)main_xn;
  (void)main_yn;
  (void)sub_xn;
  (void)sub_yn;
  (void)frames;
}

__attribute__((weak))
void _hook_suspend(void)
{

}

__attribute__((weak))
void _hook_resume(void)
{

}

__attribute__((weak))
void _hook_param(uint8_t index, int32_t value)
{
  (void)index;
  (void)value;
}

/** @} */


/** @} */

