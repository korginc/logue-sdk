#pragma once
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

#include "fixed_math.h"
#include "int_math.h"
#include "float_math.h"

/**
 * @file    simplelfo.hpp
 * @brief   Simple LFO.
 *
 * @addtogroup dsp DSP
 * @{
 */

/**
 * Common DSP Utilities
 */
namespace dsp {    

  /**
   * Simple LFO
   */
  struct SimpleLFO {
    
    /*===========================================================================*/
    /* Types and Data Structures.                                                */
    /*===========================================================================*/
      
    /*===========================================================================*/
    /* Constructor / Destructor.                                                 */
    /*===========================================================================*/

    /**
     * Default constructor
     */
    SimpleLFO(void) :
      phi0(0x80000000), w0(0)
    { }
      
    /*===========================================================================*/
    /* Public Methods.                                                           */
    /*===========================================================================*/

    /**
     * Step phase one cycle forward
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void cycle(void)
    {
      phi0 += w0;
    }

    /**
     * Reset phase
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void reset(void) 
    {
      phi0 = 0x80000000;
    }

    /**
     * Set LFO frequency
     *
     * param f0 Frequency in Hz
     * param fsrecip Reciprocal of sampling frequency (1/Fs)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setF0(const float f0, const float fsrecip) 
    {
      w0 = f32_to_q31(2.f * f0 * fsrecip);
    }

    /**
     * Set LFO frequency in radians
     *
     * @param w Frequency in radians
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setW0(const float w) 
    {
      w0 = f32_to_q31(2.f * w);
    }
    
    // --- Sinusoids --------------

    /**
     * Get value of bipolar sine wave for current phase 
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float sine_bi(void) 
    {
      const float phif = q31_to_f32(phi0);
      return 4 * phif * (si_fabsf(phif) - 1.f);
    }          

    /**
     * Get value of positive unipolar sine wave for current phase 
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float sine_uni(void) 
    {
      const float phif = q31_to_f32(phi0);
      return 0.5f + 2 * phif * (si_fabsf(phif) - 1.f);
    }          

    /**
     * Get current value of bipolar sine wave for phase with offset
     *
     * @param offset Offset to apply to current phase, in [-1, 1]
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float sine_bi_off(const float offset) 
    {
      const float phi = q31_to_f32(phi0 + f32_to_q31(2*offset));
      return 4 * phi * (si_fabsf(phi) - 1.f);
    }          

    /**
     * Get current value of positive unipolar sine wave for phase with offset
     *
     * @param offset Offset to apply to current phase, in [-1, 1]
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float sine_uni_off(const float offset) 
    {
      const float phi = q31_to_f32(phi0 + f32_to_q31(2*offset));
      return 0.5f + 2 * phi * (si_fabsf(phi) - 1.f);
    }          

    // --- Triangles --------------
    /**
     * Get current value of bipolar triangle wave for current phase
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float triangle_bi(void) 
    {
      return q31_to_f32(qsub(q31abs(phi0),0x40000000)<<1);
    }          

    /**
     * Get value of positive unipolar triangle wave for current phase 
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float triangle_uni(void) 
    {
      return si_fabsf(q31_to_f32(phi0));
    }          

    /**
     * Get current value of bipolar triangle wave for phase with offset
     *
     * @param offset Offset to apply to current phase, in [-1, 1]
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float triangle_bi_off(const float offset) 
    {
      const q31_t phi = phi0 + f32_to_q31(2*offset);
      return q31_to_f32(qsub(q31abs(phi),0x40000000)<<1);
    }          

    /**
     * Get current value of positive unipolar triangle wave for phase with offset
     *
     * @param offset Offset to apply to current phase, in [-1, 1]
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float triangle_uni_off(const float offset) 
    {
      const float phi = q31_to_f32(phi0 + f32_to_q31(2*offset));
      return si_fabsf(phi); 
    }
      
    // --- Saws --------------
    /**
     * Get current value of bipolar saw wave for current phase
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float saw_bi(void) 
    {
      return q31_to_f32(phi0);
    }          

    /**
     * Get value of positive unipolar saw wave for current phase 
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float saw_uni(void) 
    {
      return q31_to_f32(qadd((phi0>>1),0x40000000));
    }          

    /**
     * Get current value of bipolar saw wave for phase with offset
     *
     * @param offset Offset to apply to current phase, in [-1, 1]
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float saw_bi_off(const float offset) 
    {
      const q31_t phi = phi0 + (f32_to_q31(offset)<<1);
      return q31_to_f32(phi);
    }          

    /**
     * Get current value of positive unipolar saw wave for phase with offset
     *
     * @param offset Offset to apply to current phase, in [-1, 1]
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float saw_uni_off(const float offset) 
    {
      q31_t phi = phi0 + (f32_to_q31(offset)<<1);        
      return 0.5f * phi + 0.5f;
      phi >>= 1;
      return q31_to_f32(qadd(phi,0x40000000));
    }
      
    // --- Squares --------------
    /**
     * Get current value of bipolar square wave for current phase
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float square_bi(void) 
    {
      return (phi0 < 0) ? -1.f : 1.f;
    }          

    /**
     * Get value of positive unipolar square wave for current phase 
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float square_uni(void) 
    {
      return (phi0 < 0) ? 0.f : 1.f;
    }          

    /**
     * Get current value of bipolar square wave for phase with offset
     *
     * @param offset Offset to apply to current phase, in [-1, 1]
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float square_bi_off(const float offset) 
    {
      const q31_t phi = phi0 + (f32_to_q31(offset)<<1);
      return (phi < 0) ? -1.f : 1.f;
    }          

    /**
     * Get current value of positive unipolar square wave for phase with offset
     *
     * @param offset Offset to apply to current phase, in [-1, 1]
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float square_uni_off(const float offset) 
    {
      const q31_t phi = phi0 + (f32_to_q31(offset)<<1);
      return (phi < 0) ? 0.f : 1.f;
    }
      
    /*===========================================================================*/
    /* Members Vars                                                              */
    /*===========================================================================*/
      
    q31_t phi0;
    q31_t w0;
      
  };
}

/** @} */
