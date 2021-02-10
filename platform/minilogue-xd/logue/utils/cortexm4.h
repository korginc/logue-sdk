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
 * @file    cortexm4.h
 * @brief   ARM Cortex-M4 specific header.
 *
 * @addtogroup utils Utils
 * @{
 *
 * @addtogroup utils_cortexm4 ARM Cortex-M4 Specific
 * @{
 */

#ifndef __cortexm4_h
#define __cortexm4_h

#include "arm_math.h" // CMSIS

/**
 * @name    ARM Cortex-M4 Core Intrinsics
 * @note    See http://www.keil.com/pack/doc/cmsis/Core/html/group__intrinsic__CPU__gr.html
 * @{
 */

#define bkpt   __BKPT
#define clrex  __CLREX
#define clz    __CLZ
#define dmb    __DMB
#define dsb    __DSB
#define isb    __ISB
#define lda    __LDA
#define ldab   __LDAB
#define ldaex  __LDAEX
#define ldaxb  __LDAEXB
#define ldaxh  __LDAEXH
#define ldah   __LDAH
#define ldrbt  __LDRBT
#define ldrexb __LDREXB
#define ldrexh __LDREXH
#define ldrexw __LDREXW
#define ldrht  __LDRHT
#define ldrt   __LDRT
#define nop    __NOP
#define rbit   __RBIT
#define rev    __REV
#define rev16  __REV16
#define revsh  __REVSH
#define ror    __ROR
#define rrx    __RRX
#define sev    __SEV
#define ssat   __SSAT
#define stl    __STL
#define stlb   __STLB
#define stlex  __STLEX
#define stlexb __STLEXB
#define stlexh __STLEXH
#define stlh   __STLH
#define strbt  __STRBT
#define strexb __STREXB
#define strexh __STREXH
#define strexw __STREXW
#define strht  __STRHT
#define strt   __STRT
#define usat   __USAT
#define wfe    __WFE
#define wfi    __WFI

/** @} */

/**
 * @name    APSR
 * @{
 */

#define apsr() __get_APSR()
#define apsr_clr(m) {                                                   \
    uint32_t p;                                                         \
    __asm__ volatile ("mrs %0, APSR\r\n"                                \
                      "bic %0, %0, %1\r\n"                              \
                      "msr APSR_nzcvq, %0\r\n" : "=r" (p) : "i" ((m))); \
  }

/** @} */

/**
 * @name ARM Cortex-M4 SIMD Intrinsics
 * @note See http://www.keil.com/pack/doc/cmsis/Core/html/group__intrinsic__SIMD__gr.html and http://www.keil.com/support/man/docs/ARMASM/armasm_dom1361289850039.htm
 * @{
 */

#define simd32_t __SIMD32_TYPE

#define sadd8   __SADD8
#define qadd8   __QADD8
#define shadd8  __SHADD8
#define uadd8   __UADD8
#define uqadd8  __UQADD8
#define uhadd8  __UHADD8
#define ssub8   __SSUB8
#define qsub8   __QSUB8
#define shsub8  __SHSUB8
#define usub8   __USUB8
#define uqsub8  __UQSUB8
#define uhsub8  __UHSUB8
#define sadd16  __SADD16
#define qadd16  __QADD16
#define shadd16 __SHADD16
#define uadd16  __UADD16
#define uqadd16 __UQADD16
#define uhadd16 __UHADD16
#define ssub16  __SSUB16
#define qsub16  __QSUB16
#define shsub16 __SHSUB16
#define usub16  __USUB16
#define uqsub16 __UQSUB16
#define uhsub16 __UHSUB16
#define sasx    __SASX
#define qasx    __QASX
#define shasx   __SHASX
#define uasx    __UASX
#define uqasx   __UQASX
#define uhasx   __UHASX
#define ssax    __SSAX
#define qsax    __QSAX
#define shsax   __SHSAX
#define usax    __USAX
#define uqsax   __UQSAX
#define uhsax   __UHSAX
#define usad8   __USAD8
#define usada8  __USADA8
#define ssat16  __SSAT16
#define usat16  __USAT16
#define uxtb16  __UXTB16
#define uxtab16 __UXTAB16
#define sxtb16  __SXTB16
#define sxtab16 __SXTAB16
#define smuad   __SMUAD
#define smuadx  __SMUADX
#define smlad   __SMLAD
#define smladx  __SMLADX
#define smlald  __SMLALD
#define smlaldx __SMLALDX
#define smusd   __SMUSD
#define smusdx  __SMUSDX
#define smlsd   __SMLSD
#define smlsdx  __SMLSDX
#define smlsld  __SMLSLD
#define smlsldx __SMLSLDX
#define sel     __SEL
#define qadd    __QADD
#define qsub    __QSUB
#define pkhbt   __PKHBT
#define pkhtb   __PKHTB
#define smmla   __SMMLA

/** @} */

#endif // __cortexm4_h

/** @} @} */
