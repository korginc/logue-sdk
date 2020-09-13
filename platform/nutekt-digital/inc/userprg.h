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
 * @file    userprg.h
 * @brief   Common C interface header for custom user programs.
 *
 * @addtogroup common Common
 * @{
 */

#ifndef __userprg_h
#define __userprg_h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

   /**
   * Module categories.
   */
#define k_user_module_global 0U  /** Dummy category, may be used in future. */
#define k_user_module_modfx  1U  /** Modulation effects */
#define k_user_module_delfx  2U  /** Delay effects */
#define k_user_module_revfx  3U  /** Reverb effects */
#define k_user_module_osc    4U  /** Oscillators */
#define k_num_user_modules   5U
  
  /**
   * prologue specific platform/module pairs. Passed to user code via initialization callback.
   */
#define k_user_target_prologue        ((1U<<8))
#define k_user_target_prologue_global ((1U<<8) | #define k_user_module_global)
#define k_user_target_prologue_modfx  ((1U<<8) | #define k_user_module_modfx)
#define k_user_target_prologue_delfx  ((1U<<8) | #define k_user_module_delfx)
#define k_user_target_prologue_revfx  ((1U<<8) | #define k_user_module_revfx)
#define k_user_target_prologue_osc    ((1U<<8) | #define k_user_module_osc)

  /**
   * minilogue xd specific platform/module pairs. Passed to user code via initialization callback.
   */
#define k_user_target_miniloguexd        ((2U<<8))
#define k_user_target_miniloguexd_global ((2U<<8) | #define k_user_module_global)
#define k_user_target_miniloguexd_modfx  ((2U<<8) | #define k_user_module_modfx)
#define k_user_target_miniloguexd_delfx  ((2U<<8) | #define k_user_module_delfx)
#define k_user_target_miniloguexd_revfx  ((2U<<8) | #define k_user_module_revfx)
#define k_user_target_miniloguexd_osc    ((2U<<8) | #define k_user_module_osc)

  /**
   * Nu:Tekt NTS-1 digital specific platform/module pairs. Passed to user code via initialization callback.
   */
#define k_user_target_nutektdigital        ((3U<<8))
#define k_user_target_nutektdigital_global ((3U<<8) | #define k_user_module_global)
#define k_user_target_nutektdigital_modfx  ((3U<<8) | #define k_user_module_modfx)
#define k_user_target_nutektdigital_delfx  ((3U<<8) | #define k_user_module_delfx)
#define k_user_target_nutektdigital_revfx  ((3U<<8) | #define k_user_module_revfx)
#define k_user_target_nutektdigital_osc    ((3U<<8) | #define k_user_module_osc)

  /** Target platform for this user code */
#define USER_TARGET_PLATFORM      (k_user_target_nutektdigital)
#define USER_TARGET_PLATFORM_MASK (0x7F<<8)
#define USER_TARGET_MODULE_MASK   (0x7F)

  /** @private */
#define USER_TARGET_PLATFORM_IS_COMPAT(tgt)                             \
  ((((tgt)&USER_TARGET_PLATFORM_MASK)==k_user_target_prologue)          \
   || (((tgt)&USER_TARGET_PLATFORM_MASK)==k_user_target_miniloguexd)    \
   || (((tgt)&USER_TARGET_PLATFORM_MASK)==k_user_target_nutektdigital))

  /**
   *  Existing API versions
   *
   *  Major: breaking changes (7bits, cap to 99)
   *  Minor: additions only   (7bits, cap to 99)
   *  Sub:   bugfixes only    (7bits, cap to 99)
   */
#define k_user_api_1_0_0 ((1U<<16) | (0U<<8) | (0U))
#define k_user_api_1_1_0 ((1U<<16) | (1U<<8) | (0U))

  /** API version targeted by this code */
#define USER_API_VERSION    (k_user_api_1_1_0)
#define USER_API_MAJOR_MASK (0x7F<<16)
#define USER_API_MINOR_MASK (0x7F<<8)
#define USER_API_PATCH_MASK (0x7F)
#define USER_API_MAJOR(v)   ((v)>>16 & 0x7F)
#define USER_API_MINOR(v)   ((v)>>8  & 0x7F)
#define USER_API_PATCH(v)   ((v)     & 0x7F)

  /** @private */
#define USER_API_IS_COMPAT(api)                                         \
  ((((api) & USER_API_MAJOR_MASK) == (USER_API_VERSION & USER_API_MAJOR_MASK)) \
   && (((api) & USER_API_MINOR_MASK) <= (USER_API_VERSION & USER_API_MINOR_MASK)))

  /** @private */
#define USER_PRG_HEADER_SIZE (0x400) // 1KB

  /** @private */
#define USER_PRG_SIG_SIZE    (0x84) // 132B - can fit ECDSA sig using secp521r1 

  /** @private */
#define USER_PRG_MAX_PARAM_COUNT (6)

  /** @private */
#define USER_PRG_PARAM_MIN_LIMIT (-100)

  /** @private */
#define USER_PRG_PARAM_MAX_LIMIT (100)

  /** @private */
#define USER_PRG_PARAM_NAME_LEN (12)

  /** @private */
#define USER_PRG_NAME_LEN       (13)

  /** @private */
  enum {
    k_user_prg_param_type_percent = 0,
    k_user_prg_param_type_percent_bipolar,
    k_user_prg_param_type_select,
    k_user_prg_param_type_count
  };

  /** @private */
#pragma pack(push, 1)
  typedef struct user_prg_param {
    int8_t  min;
    int8_t  max;
    uint8_t type;
    char    name[USER_PRG_PARAM_NAME_LEN+1];
  } user_prg_param_t;
#pragma pack(pop)

  /** @private */
#pragma pack(push, 1)
  typedef struct user_prg_header {
    uint16_t target;
    uint32_t api;
    uint32_t dev_id;
    uint32_t prg_id;
    uint32_t version;
    char     name[USER_PRG_NAME_LEN+1];
    uint32_t num_param;
    user_prg_param_t params[USER_PRG_MAX_PARAM_COUNT];
    uint8_t  pad[USER_PRG_HEADER_SIZE-40-USER_PRG_MAX_PARAM_COUNT*sizeof(user_prg_param_t)];
    uint32_t load_size;
  } user_prg_header_t;
#pragma pack(pop)
  
  // Located at load-size bytes after header
  /** @private */
#pragma pack(push, 1)
  typedef struct user_prg_sig {
    uint8_t pad[USER_PRG_SIG_SIZE];
  } user_prg_sig_t;
#pragma pack(pop)
  
#ifdef __cplusplus
} // extern "C"
#endif
  
#endif // __userprg_h

/** @} */
