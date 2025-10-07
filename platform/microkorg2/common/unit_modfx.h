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
 *  @file unit_modfx.h
 *
 *  @brief Base header for modulation effect units
 *
 */

#ifndef UNIT_MODFX_H_
#define UNIT_MODFX_H_

#include <stdint.h>

#include "unit.h"
#include "fx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

  #define MODFX_MEMORY_SIZE_BYTES 0x10000U
  #define MODFX_MEMORY_SIZE (MODFX_MEMORY_SIZE_BYTES >> 2) // assumes 32 bit data type

  /** Exposed parameters with fixed/direct UI controls. */
  enum {
    k_unit_modfx_fixed_param_time = 0,
    k_unit_modfx_fixed_param_depth,
    k_num_unit_modfx_fixed_param_id
  };

#define UNIT_MODFX_MAX_PARAM_COUNT (8)
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif // UNIT_MODFX_H_
