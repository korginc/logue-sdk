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
 * @brief   Oscillator entry template.
 *
 * @addtogroup api
 * @{
 */

#include "userosc.h"

/*===========================================================================*/
/* Local Constants and Vars.                                                 */
/*===========================================================================*/

/**
 * @name   Local Constants and Vars.
 * @{
 */

__attribute__((used, section(".hooks")))
static const user_osc_hook_table_t s_hook_table = {
  .magic = {'U','O','S','C'},
  .padding = {0},
  .func_init = _hook_init,
  .func_cycle = _hook_cycle,
  .func_on = _hook_on,
  .func_off = _hook_off,
  .func_mute = _hook_mute,
  .func_value = _hook_value,
  .func_param = _hook_param,
};

/** @} */

/*===========================================================================*/
/* Default Hooks.                                                             */
/*===========================================================================*/

/**
 * @name   Default Hooks.
 * @{
 */

__attribute__((weak))
void _hook_init(uint32_t platform, uint32_t api)
{
  (void)platform;
  (void)api;
}

__attribute__((weak))
void _hook_cycle(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames)
{
  (void)params;
  (void)yn;
  (void)frames;
}

__attribute__((weak))
void _hook_on(const user_osc_param_t * const params)
{
  (void)params;
}

__attribute__((weak))
void _hook_off(const user_osc_param_t * const params)
{
  (void)params;
}

__attribute__((weak))
void _hook_mute(const user_osc_param_t * const params)
{
  (void)params;
}

__attribute__((weak))
void _hook_value(uint16_t value)
{
  (void)value;
}

__attribute__((weak))
void _hook_param(uint16_t index, uint16_t value)
{
  (void)index;
  (void)value;
}

/** @} */


/** @} */
