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
 * @brief   Delay effect entry template.
 *
 * @addtogroup api
 * @{
 */

#include "userdelfx.h"

/*===========================================================================*/
/* Local Constants and Vars.                                                 */
/*===========================================================================*/

/**
 * @name   Local Constants and Vars.
 * @{
 */

__attribute__((used, section(".hooks")))
static const user_delfx_hook_table_t s_hook_table = {
  .magic = {'U','D','E','L'},
  .padding = {0},
  .func_init = _hook_init,
  .func_process = _hook_process,
  .func_suspend = _hook_suspend,
  .func_resume = _hook_resume,
  .func_param = _hook_param
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
void _hook_process(float *xn, uint32_t frames)
{
  (void)xn;
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
