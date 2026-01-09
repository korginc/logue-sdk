#pragma once

#include <stdio.h>

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
 * @file    mk2_biquad.hpp
 * @brief   Generic biquad structure and convenience methods.
 *
 * @addtogroup dsp DSP
 * @{
 *
 */

#include "dsp/biquad.hpp"
#include "attributes.h"
#include "utils/float_simd.h"
#include "utils/buffer_ops.h"

/**
 * Common DSP Utilities
 */
namespace dsp
{
  template <int NumParallelFilters> struct ParallelBiQuad 
  {
    struct ParallelCoeffs {
      float ff0[NumParallelFilters];
      float ff1[NumParallelFilters];
      float ff2[NumParallelFilters];
      float fb1[NumParallelFilters];
      float fb2[NumParallelFilters];
      
      /**
       * Default constructor
       */
      ParallelCoeffs()
      { 
        // flush
        buf_clr_f32(ff0, NumParallelFilters);
        buf_clr_f32(ff1, NumParallelFilters);
        buf_clr_f32(ff2, NumParallelFilters);
        buf_clr_f32(fb1, NumParallelFilters);
        buf_clr_f32(fb2, NumParallelFilters);
      }

      void set_coeffs(BiQuad::Coeffs coeffs, int filterNumber)
      {
        ff0[filterNumber] = coeffs.ff0;
        ff1[filterNumber] = coeffs.ff1;
        ff2[filterNumber] = coeffs.ff2;
        fb1[filterNumber] = coeffs.fb1;
        fb2[filterNumber] = coeffs.fb2;
      }
    };
    /*=====================================================================*/
    /* Constructor / Destructor.                                           */
    /*=====================================================================*/

    /**
     * Default constructor
     */
    ParallelBiQuad(void):
    mVectorx4Loops(NumParallelFilters / 4),
    mVectorx2Loops((NumParallelFilters - (mVectorx4Loops * 4)) / 2),
    mVectorx1Loops(NumParallelFilters - (mVectorx4Loops * 4) - (mVectorx2Loops * 2))
    { }
      
    /*=====================================================================*/
    /* Public Methods.                                                     */
    /*=====================================================================*/

    /**
     * Flush internal delays
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void flush(void) {
      buf_clr_f32(mZ1, NumParallelFilters);
      buf_clr_f32(mZ2, NumParallelFilters);
    }

    /**
     * Second order processing of one sample
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float process_so_x1(const float xn, int filterNumber = 0) {
      float acc = mCoeffs.ff0 * xn + mZ1[filterNumber];
      mZ1[filterNumber] = mCoeffs.ff1 * xn + mZ2[filterNumber];
      mZ2[filterNumber] = mCoeffs.ff2 * xn;
      mZ1[filterNumber] -= mCoeffs.fb1 * acc;
      mZ2[filterNumber] -= mCoeffs.fb2 * acc;
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float process_so_x1(const float xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float acc = coeffs.ff0[filterNumber] * xn + mZ1[filterNumber];
      mZ1[filterNumber] = coeffs.ff1[filterNumber] * xn + mZ2[filterNumber];
      mZ2[filterNumber] = coeffs.ff2[filterNumber] * xn;
      mZ1[filterNumber] -= coeffs.fb1[filterNumber] * acc;
      mZ2[filterNumber] -= coeffs.fb2[filterNumber] * acc;
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process_so_x2(const float32x2_t xn, int filterNumber = 0) {
      float32x2_t acc = float32x2_fmuladd(f32x2_ld(&mZ1[filterNumber]), f32x2_dup(mCoeffs.ff0), xn);
      float32x2_t z1 = float32x2_fmuladd(f32x2_ld(&mZ2[filterNumber]), f32x2_dup(mCoeffs.ff1), xn);
      float32x2_t z2 = float32x2_mul(f32x2_dup(mCoeffs.ff2), xn);
      f32x2_str(&mZ1[filterNumber], float32x2_fmulsub(z1, f32x2_dup(mCoeffs.fb1), acc));
      f32x2_str(&mZ2[filterNumber], float32x2_fmulsub(z2, f32x2_dup(mCoeffs.fb2), acc));
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process_so_x2(const float32x2_t xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float32x2_t acc = float32x2_fmuladd(f32x2_ld(&mZ1[filterNumber]), f32x2_ld(&coeffs.ff0[filterNumber]), xn);
      float32x2_t z1 = float32x2_fmuladd(f32x2_ld(&mZ2[filterNumber]), f32x2_ld(&coeffs.ff1[filterNumber]), xn);
      float32x2_t z2 = float32x2_mul(f32x2_ld(&coeffs.ff2[filterNumber]), xn);
      f32x2_str(&mZ1[filterNumber], float32x2_fmulsub(z1, f32x2_ld(&coeffs.fb1[filterNumber]), acc));
      f32x2_str(&mZ2[filterNumber], float32x2_fmulsub(z2, f32x2_ld(&coeffs.fb2[filterNumber]), acc));
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process_so_x4(const float32x4_t xn, int filterNumber = 0) {
      float32x4_t acc = float32x4_fmuladd(f32x4_ld(&mZ1[filterNumber]), f32x4_dup(mCoeffs.ff0), xn);
      float32x4_t z1 = float32x4_fmuladd(f32x4_ld(&mZ2[filterNumber]), f32x4_dup(mCoeffs.ff1), xn);
      float32x4_t z2 = float32x4_mul(f32x4_dup(mCoeffs.ff2), xn);
      f32x4_str(&mZ1[filterNumber], float32x4_fmulsub(z1, f32x4_dup(mCoeffs.fb1), acc));
      f32x4_str(&mZ2[filterNumber], float32x4_fmulsub(z2, f32x4_dup(mCoeffs.fb2), acc));
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process_so_x4(const float32x4_t xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float32x4_t acc = float32x4_fmuladd(f32x4_ld(&mZ1[filterNumber]), f32x4_ld(&coeffs.ff0[filterNumber]), xn);
      float32x4_t z1 = float32x4_fmuladd(f32x4_ld(&mZ2[filterNumber]), f32x4_ld(&coeffs.ff1[filterNumber]), xn);
      float32x4_t z2 = float32x4_mul(f32x4_ld(&coeffs.ff2[filterNumber]), xn);
      f32x4_str(&mZ1[filterNumber], float32x4_fmulsub(z1, f32x4_ld(&coeffs.fb1[filterNumber]), acc));
      f32x4_str(&mZ2[filterNumber], float32x4_fmulsub(z2, f32x4_ld(&coeffs.fb2[filterNumber]), acc));
      return acc;
    }

    /**
     * First order processing of one sample
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float process_fo_x1(const float xn, int filterNumber = 0) {
      float acc = mCoeffs.ff0 * xn + mZ1[filterNumber];
      mZ1[filterNumber] = mCoeffs.ff1 * xn;
      mZ1[filterNumber] -= mCoeffs.fb1 * acc;
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float process_fo_x1(const float xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float acc = coeffs.ff0[filterNumber] * xn + mZ1[filterNumber];
      mZ1[filterNumber] = coeffs.ff1[filterNumber] * xn;
      mZ1[filterNumber] -= coeffs.fb1[filterNumber] * acc;
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process_fo_x2(const float32x2_t xn, int filterNumber = 0) {
      float32x2_t acc = float32x2_fmuladd(f32x2_ld(&mZ1[filterNumber]), f32x2_dup(mCoeffs.ff0), xn);
      float32x2_t z1 = float32x2_mul(f32x2_dup(mCoeffs.ff1), xn);
      f32x2_str(&mZ1[filterNumber], float32x2_fmulsub(z1, f32x2_dup(mCoeffs.fb1), acc));
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process_fo_x2(const float32x2_t xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float32x2_t acc = float32x2_fmuladd(f32x2_ld(&mZ1[filterNumber]), f32x2_ld(coeffs.ff0[filterNumber]), xn);
      float32x2_t z1 = float32x2_mul(f32x2_ld(coeffs.ff1[filterNumber]), xn);
      f32x2_str(&mZ1[filterNumber], float32x2_fmulsub(z1, f32x2_ld(coeffs.fb1[filterNumber]), acc));
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process_fo_x4(const float32x4_t xn, int filterNumber = 0) {
      float32x4_t acc = float32x4_fmuladd(f32x4_ld(&mZ1[filterNumber]), f32x4_dup(mCoeffs.ff0), xn);
      float32x4_t z1 = float32x4_mul(f32x4_dup(mCoeffs.ff1), xn);
      f32x4_str(&mZ1[filterNumber], float32x4_fmulsub(z1, f32x4_dup(mCoeffs.fb1), acc));
      return acc;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process_fo_x4(const float32x4_t xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float32x4_t acc = float32x4_fmuladd(f32x4_ld(&mZ1[filterNumber]), f32x4_ld(coeffs.ff0[filterNumber]), xn);
      float32x4_t z1 = float32x4_mul(f32x4_ld(coeffs.ff1[filterNumber]), xn);
      f32x4_str(&mZ1[filterNumber], float32x4_fmulsub(z1, f32x4_ld(coeffs.fb1[filterNumber]), acc));
      return acc;
    }

    /**
     * Default processing function (second order)
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float process(const float xn) {
      return process_so_x1(xn);
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float process(const float xn, ParallelCoeffs & coeffs) {
      return process_so_x1(xn, coeffs);
    }
      
    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process(const float32x2_t xn, ParallelCoeffs & coeffs) {
      return process_so_x2(xn, coeffs);
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process(const float32x4_t xn, ParallelCoeffs & coeffs) {
      return process_so_x4(xn, coeffs);
    }

    // Assumes the size of the input array xn is the same size as the number of parallel filters.
    // Processes as second order to avoid needing to check for filter type for each band.
    // Processes in place.
    inline __attribute__((optimize("Ofast"),always_inline))
    void process(const float * xn) {
      int filterNumber = 0;
      for(int i = 0; i < mVectorx4Loops; i++)
      {
        f32x4_str(xn, process_so_x4(xn, filterNumber));
        xn += 4;
        filterNumber += 4;
      }

      for(int i = 0; i < mVectorx2Loops; i++)
      {
        f32x2_str(xn, process_so_x2(xn, filterNumber));
        xn += 2;
        filterNumber += 2;
      }

      for(int i = 0; i < mVectorx1Loops; i++)
      {
        *xn = process_so_x1(xn, filterNumber);
        xn += 1;
        filterNumber += 1;
      }
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    void process(const float * xn, ParallelCoeffs & coeffs) {
      int filterNumber = 0;
      for(int i = 0; i < mVectorx4Loops; i++)
      {
        f32x4_str(xn, process_so_x4(xn, coeffs, filterNumber));
        xn += 4;
        filterNumber += 4;
      }

      for(int i = 0; i < mVectorx2Loops; i++)
      {
        f32x2_str(xn, process_so_x2(xn, coeffs, filterNumber));
        xn += 2;
        filterNumber += 2;
      }

      for(int i = 0; i < mVectorx1Loops; i++)
      {
        *xn = process_so_x1(xn, coeffs, filterNumber);
        xn += 1;
        filterNumber += 1;
      }
    }

    /*=====================================================================*/
    /* Member Variables.                                                   */
    /*=====================================================================*/

    /** Maintain compatibility with non-parallel version */
    BiQuad::Coeffs mCoeffs;
    float mZ1[NumParallelFilters];
    float mZ2[NumParallelFilters];   
    
    const uint32_t mVectorx4Loops;
    const uint32_t mVectorx2Loops;
    const uint32_t mVectorx1Loops;
  };

  // Extended BiQuad structure
  template <int NumParallelFilters> struct ParallelExtBiQuad {

    // coefficient structure to support different coeffs per filter
    struct ParallelCoeffs {
      float ff0[NumParallelFilters];
      float ff1[NumParallelFilters];
      float ff2[NumParallelFilters];
      float fb1[NumParallelFilters];
      float fb2[NumParallelFilters];
      float d0[NumParallelFilters];
      float d1[NumParallelFilters]; 
      float w0[NumParallelFilters]; 
      float w1[NumParallelFilters];
  
      /**
       * Default constructor
       */
      ParallelCoeffs()
      { 
        // flush
        buf_clr_f32(ff0, NumParallelFilters);
        buf_clr_f32(ff1, NumParallelFilters);
        buf_clr_f32(ff2, NumParallelFilters);
        buf_clr_f32(fb1, NumParallelFilters);
        buf_clr_f32(fb2, NumParallelFilters);

        buf_clr_f32(d0, NumParallelFilters);
        buf_clr_f32(d1, NumParallelFilters);
        buf_clr_f32(w0, NumParallelFilters);
        buf_clr_f32(w1, NumParallelFilters);
      }

      void set_coeffs(ExtBiQuad & coeffFilter, int filterNumber)
      {
        ff0[filterNumber] = coeffFilter.mCoeffs.ff0;
        ff1[filterNumber] = coeffFilter.mCoeffs.ff1;
        ff2[filterNumber] = coeffFilter.mCoeffs.ff2;
        fb1[filterNumber] = coeffFilter.mCoeffs.fb1;
        fb2[filterNumber] = coeffFilter.mCoeffs.fb2;

        d0[filterNumber] = coeffFilter.mD0;
        d1[filterNumber] = coeffFilter.mD1; 
        w0[filterNumber] = coeffFilter.mW0; 
        w1[filterNumber] = coeffFilter.mW1;
      }

      void set_coeffs(ParallelExtBiQuad * coeffs)
      {
        buf_cpy_f32(coeffs, ff0, NumParallelFilters);
        buf_cpy_f32(coeffs, ff1, NumParallelFilters);
        buf_cpy_f32(coeffs, ff2, NumParallelFilters);
        buf_cpy_f32(coeffs, fb1, NumParallelFilters);
        buf_cpy_f32(coeffs, fb2, NumParallelFilters);

        buf_cpy_f32(coeffs, d0, NumParallelFilters);
        buf_cpy_f32(coeffs, d1, NumParallelFilters);
        buf_cpy_f32(coeffs, w0, NumParallelFilters);
        buf_cpy_f32(coeffs, w1, NumParallelFilters);
      }
    };
    /*=====================================================================*/
    /* Types and Data Structures.                                          */
    /*=====================================================================*/
            
    /*=====================================================================*/
    /* Constructor / Destructor.                                           */
    /*=====================================================================*/

    /**
     * Default constructor.
     */
    ParallelExtBiQuad(void) :
      mD0(0), mD1(0),
      mW0(0), mW1(0),
      mVectorx4Loops(NumParallelFilters / 4),
      mVectorx2Loops((NumParallelFilters - (mVectorx4Loops * 4)) / 2),
      mVectorx1Loops(NumParallelFilters - (mVectorx4Loops * 4) - (mVectorx2Loops * 2))
    { }

        /**
     * Flush internal delays
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void flush(void) {
      buf_clr_f32(mZ1, NumParallelFilters);
      buf_clr_f32(mZ2, NumParallelFilters);
    }

    /**
     * Second order processing of one sample
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float process_so_x1(const float xn, int filterNumber = 0) {
      float acc = mCoeffs.ff0 * xn + mZ1[filterNumber];
      mZ1[filterNumber] = mCoeffs.ff1 * xn + mZ2[filterNumber];
      mZ2[filterNumber] = mCoeffs.ff2 * xn;
      mZ1[filterNumber] -= mCoeffs.fb1 * acc;
      mZ2[filterNumber] -= mCoeffs.fb2 * acc;
      return mW1 * (mW0 * acc + mD0 * xn) + mD1 * xn;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float process_so_x1(const float xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float acc = coeffs.ff0[filterNumber] * xn + mZ1[filterNumber];
      mZ1[filterNumber] = coeffs.ff1[filterNumber] * xn + mZ2[filterNumber];
      mZ2[filterNumber] = coeffs.ff2[filterNumber] * xn;
      mZ1[filterNumber] -= coeffs.fb1[filterNumber] * acc;
      mZ2[filterNumber] -= coeffs.fb2[filterNumber] * acc;
      return coeffs.w1[filterNumber] * (coeffs.w0[filterNumber] * acc + coeffs.d0[filterNumber] * xn) + coeffs.d1[filterNumber] * xn;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process_so_x2(const float32x2_t xn, int filterNumber) {
      float32x2_t acc = float32x2_fmuladd(f32x2_ld(&mZ1[filterNumber]), f32x2_dup(mCoeffs.ff0), xn);
      float32x2_t z1 = float32x2_fmuladd(f32x2_ld(&mZ2[filterNumber]), f32x2_dup(mCoeffs.ff1), xn);
      float32x2_t z2 = float32x2_mul(f32x2_dup(mCoeffs.ff2), xn);
      f32x2_str(&mZ1[filterNumber], float32x2_fmulsub(z1, f32x2_dup(mCoeffs.fb1), acc));
      f32x2_str(&mZ2[filterNumber], float32x2_fmulsub(z2, f32x2_dup(mCoeffs.fb2), acc));
      
      return float32x2_add(float32x2_mul(f32x2_dup(mW1),
                                         float32x2_add(float32x2_mul(f32x2_dup(mW0), acc),
                                                       float32x2_mul(f32x2_dup(mD0), xn))),
                           float32x2_mul(f32x2_dup(mD1), xn));
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process_so_x2(const float32x2_t xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float32x2_t acc = float32x2_fmuladd(f32x2_ld(&mZ1[filterNumber]), f32x2_ld(&coeffs.ff0[filterNumber]), xn);
      float32x2_t z1 = float32x2_fmuladd(f32x2_ld(&mZ2[filterNumber]), f32x2_ld(&coeffs.ff1[filterNumber]), xn);
      float32x2_t z2 = float32x2_mul(f32x2_ld(&coeffs.ff2[filterNumber]), xn);
      f32x2_str(&mZ1[filterNumber], float32x2_fmulsub(z1, f32x2_ld(&coeffs.fb1[filterNumber]), acc));
      f32x2_str(&mZ2[filterNumber], float32x2_fmulsub(z2, f32x2_ld(&coeffs.fb2[filterNumber]), acc));
      
      return float32x2_add(float32x2_mul(f32x2_ld(&coeffs.w1[filterNumber]),
                                         float32x2_add(float32x2_mul(f32x2_ld(&coeffs.w0[filterNumber]), acc),
                                                       float32x2_mul(f32x2_ld(&coeffs.d0[filterNumber]), xn))),
                           float32x2_mul(f32x2_ld(&coeffs.d1[filterNumber]), xn));
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process_so_x4(const float32x4_t xn, int filterNumber) {
      float32x4_t acc = float32x4_fmuladd(f32x4_ld(&mZ1[filterNumber]), f32x4_dup(mCoeffs.ff0), xn);
      float32x4_t z1 = float32x4_fmuladd(f32x4_ld(&mZ2[filterNumber]), f32x4_dup(mCoeffs.ff1), xn);
      float32x4_t z2 = float32x4_mul(f32x4_dup(mCoeffs.ff2), xn);
      f32x4_str(&mZ1[filterNumber], float32x4_fmulsub(z1, f32x4_dup(mCoeffs.fb1), acc));
      f32x4_str(&mZ2[filterNumber], float32x4_fmulsub(z2, f32x4_dup(mCoeffs.fb2), acc));
      
      return float32x4_add(float32x4_mul(f32x4_dup(mW1),
                                         float32x4_add(float32x4_mul(f32x4_dup(mW0), acc),
                                                       float32x4_mul(f32x4_dup(mD0), xn))),
                           float32x4_mul(f32x4_dup(mD1), xn));
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process_so_x4(const float32x4_t xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float32x4_t acc = float32x4_fmuladd(f32x4_ld(&mZ1[filterNumber]), f32x4_ld(&coeffs.ff0[filterNumber]), xn);
      float32x4_t z1 = float32x4_fmuladd(f32x4_ld(&mZ2[filterNumber]), f32x4_ld(&coeffs.ff1[filterNumber]), xn);
      float32x4_t z2 = float32x4_mul(f32x4_ld(&coeffs.ff2[filterNumber]), xn);
      f32x4_str(&mZ1[filterNumber], float32x4_fmulsub(z1, f32x4_ld(&coeffs.fb1[filterNumber]), acc));
      f32x4_str(&mZ2[filterNumber], float32x4_fmulsub(z2, f32x4_ld(&coeffs.fb2[filterNumber]), acc));
      
      return float32x4_add(float32x4_mul(f32x4_ld(&coeffs.w1[filterNumber]),
                                         float32x4_add(float32x4_mul(f32x4_ld(&coeffs.w0[filterNumber]), acc),
                                                       float32x4_mul(f32x4_ld(&coeffs.d0[filterNumber]), xn))),
                           float32x4_mul(f32x4_ld(&coeffs.d1[filterNumber]), xn));
    }

    /**
     * First order processing of one sample
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float process_fo_x1(const float xn, int filterNumber = 0) {
      float acc = mCoeffs.ff0 * xn + mZ1[filterNumber];
      mZ1[filterNumber] = mCoeffs.ff1 * xn;
      mZ1[filterNumber] -= mCoeffs.fb1 * acc;
      return mW1 * (mW0 * acc + mD0 * xn) + mD1 * xn;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float process_fo_x1(const float xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float acc = coeffs.ff0[filterNumber] * xn + mZ1[filterNumber];
      mZ1[filterNumber] = coeffs.ff1[filterNumber] * xn;
      mZ1[filterNumber] -= coeffs.fb1[filterNumber] * acc;
      return coeffs.w1[filterNumber] * (coeffs.w0[filterNumber] * acc + coeffs.d0[filterNumber] * xn) + coeffs.d1[filterNumber] * xn;
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process_fo_x2(const float32x2_t xn, int filterNumber = 0) {
      float32x2_t acc = float32x2_fmuladd(f32x2_ld(&mZ1[filterNumber]), f32x2_dup(mCoeffs.ff0), xn);
      float32x2_t z1 = float32x2_mul(f32x2_dup(mCoeffs.ff1), xn);
      f32x2_str(&mZ1[filterNumber], float32x2_fmulsub(z1, f32x2_dup(mCoeffs.fb1), acc));
      return float32x2_add(float32x2_mul(f32x2_dup(mW1),
                                         float32x2_add(float32x2_mul(f32x2_dup(mW0), acc),
                                                       float32x2_mul(f32x2_dup(mD0), xn))),
                           float32x2_mul(f32x2_dup(mD1), xn));
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x2_t process_fo_x2(const float32x2_t xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float32x2_t acc = float32x2_fmuladd(f32x2_ld(&mZ1[filterNumber]), f32x2_ld(&coeffs.ff0[filterNumber]), xn);
      float32x2_t z1 = float32x2_mul(f32x2_ld(&coeffs.ff1[filterNumber]), xn);
      f32x2_str(&mZ1[filterNumber], float32x2_fmulsub(z1, f32x2_ld(&coeffs.fb1[filterNumber]), acc));

      return float32x2_add(float32x2_mul(f32x2_ld(&coeffs.w1[filterNumber]),
                                         float32x2_add(float32x2_mul(f32x2_ld(&coeffs.w0[filterNumber]), acc),
                                                       float32x2_mul(f32x2_ld(&coeffs.d0[filterNumber]), xn))),
                           float32x2_mul(f32x2_ld(&coeffs.d1[filterNumber]), xn));
    }
    
    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process_fo_x4(const float32x4_t xn, int filterNumber = 0) {
      float32x4_t acc = float32x4_fmuladd(f32x4_ld(&mZ1[filterNumber]), f32x4_dup(mCoeffs.ff0), xn);
      float32x4_t z1 = float32x4_mul(f32x4_dup(mCoeffs.ff1), xn);
      f32x4_str(&mZ1[filterNumber], float32x4_fmulsub(z1, f32x4_dup(mCoeffs.fb1), acc));
      return float32x4_add(float32x4_mul(f32x4_dup(mW1),
                                         float32x4_add(float32x4_mul(f32x4_dup(mW0), acc),
                                                       float32x4_mul(f32x4_dup(mD0), xn))),
                           float32x4_mul(f32x4_dup(mD1), xn));
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    float32x4_t process_fo_x4(const float32x4_t xn, ParallelCoeffs & coeffs, int filterNumber = 0) {
      float32x4_t acc = float32x4_fmuladd(f32x4_ld(&mZ1[filterNumber]), f32x4_ld(&coeffs.ff0[filterNumber]), xn);
      float32x4_t z1 = float32x4_mul(f32x4_ld(&coeffs.ff1[filterNumber]), xn);
      f32x4_str(&mZ1[filterNumber], float32x4_fmulsub(z1, f32x4_ld(&coeffs.fb1[filterNumber]), acc));
      return float32x4_add(float32x4_mul(f32x4_ld(&coeffs.w1[filterNumber]),
                                         float32x4_add(float32x4_mul(f32x4_ld(&coeffs.w0[filterNumber]), acc),
                                                       float32x4_mul(f32x4_ld(&coeffs.d0[filterNumber]), xn))),
                           float32x4_mul(f32x4_ld(&coeffs.d1[filterNumber]), xn));
    }

    /**
     * Default processing function (second order)
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void process(const float * xn) {
      int filterNumber = 0;
      for(int i = 0; i < mVectorx4Loops; i++)
      {
        f32x4_str(xn, process_so_x4(xn, filterNumber));
        xn += 4;
        filterNumber += 4;
      }

      for(int i = 0; i < mVectorx2Loops; i++)
      {
        f32x2_str(xn, process_so_x2(xn, filterNumber));
        xn += 2;
        filterNumber += 2;
      }

      for(int i = 0; i < mVectorx1Loops; i++)
      {
        *xn = process_so_x1(xn, filterNumber);
        xn += 1;
        filterNumber += 1;
      }
    }

    inline __attribute__((optimize("Ofast"),always_inline))
    void process(const float * xn, ParallelCoeffs & coeffs) {
      int filterNumber = 0;
      for(int i = 0; i < mVectorx4Loops; i++)
      {
        f32x4_str(xn, process_so_x4(xn, coeffs, filterNumber));
        xn += 4;
        filterNumber += 4;
      }

      for(int i = 0; i < mVectorx2Loops; i++)
      {
        f32x2_str(xn, process_so_x2(xn, coeffs, filterNumber));
        xn += 2;
        filterNumber += 2;
      }

      for(int i = 0; i < mVectorx1Loops; i++)
      {
        *xn = process_so_x1(xn, coeffs, filterNumber);
        xn += 1;
        filterNumber += 1;
      }
    }

    // -- Invertable All-Pass based Low/High Pass (copied from biquad.hpp for compatibility) -------

    /**
     * Calculate coefficients for "invertable" all pass based low pass filter.
     *
     * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setFOAPLP(const float k) {
      // k = tan(pi*wc)
      mCoeffs.setFOAP(k);
      mD0 = mW0 = 0.5f;
      mD1 = 0.f;
      mW1 = 1.f;
    }

    /**
     * Calculate coefficients for "invertable" all pass based high pass filter.
     *
     * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setFOAPHP(const float k) {
      // k = tan(pi*wc)
      mCoeffs.setFOAP(k);
      mD0 = 0.5f;
      mW0 = -0.5f;
      mD1 = 0.f;
      mW1 = 1.f;
    }

    /**
     * Toggle "invertable" all pass based low/high pass filter to opposite mode.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void toggleFOLPHP(void) {
      mW0 = -mW0;
    }

    /**
     * Update "invertable" all pass based low/high pass filter coefficients. Agnostic from current mode.
     *
     * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void updateFOLPHP(const float k) {
      mCoeffs.setFOAP(k);
    }

    // -- All-Pass based Low/High Shelf -----------------
    
    /**
     * Calculate coefficients for first order all pass based low shelf filter.
     *
     * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
     * @param   gain 10^(gain_db/20)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setFOLS(const float k, const float gain) {
      // k = tan(pi*wc)
      // gain = raw amplitude (pre converted from dB)
      const float h = gain - 1.f;
      const float g = (gain >=  1.f) ? 1.f : gain;
      mCoeffs.ff0 = mCoeffs.fb1 = (k - g) / (k + g);
      mCoeffs.ff1 = 1.f;
      mCoeffs.fb2 = mCoeffs.ff2 = 0.f;

      mW0 = 1.f;
      mD0 = 1.f;
        
      mW1 = 0.5f * h;
      mD1 = 1.f;
    }

    /**
     * Calculate coefficients for first order all pass based high shelf filter.
     *
     * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
     * @param   gain 10^(gain_db/20)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setFOHS(const float k, const float gain) {
      // k = tan(pi*wc)
      // gain = raw amplitude (pre converted from dB)
      const float h = gain - 1.f;
      const float gk = (gain >=  1.f) ? k : gain*k;
      mCoeffs.ff0 = mCoeffs.fb1 = (gk - 1.f) / (gk + 1);
      mCoeffs.ff1 = 1.f;
      mCoeffs.fb2 = mCoeffs.ff2 = 0.f;

      mW0 = -1.f;
      mD0 = 1.f;
        
      mW1 = 0.5f * h;
      mD1 = 1.f;
    }

    // TODO: second order shelves
      
    // -- All-Pass based Band Pass/Reject -------------
    /**
     * Calculate coefficients for second order all pass based band reject filter.
     *
     * @param   delta cos(2pi*wc)
     * @param   gamma tan(pi * wb)
     *
     * @note q is inverse of relative bandwidth (wc / wb)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setSOAPBR2(const float delta, const float gamma) {
      // Alternative implementation based on second order tunable all pass
      // delta = cos(2pi*wc)
      mCoeffs.setSOAP2(delta, gamma);

      mW0 = 1.f;
      mD0 = 1.f;

      mW1 = 0.5f;
      mD1 = 0.f;
    }

    /**
     * Calculate coefficients for second order all pass based band pass filter.
     *
     * @param   delta cos(2pi*wc)
     * @param   gamma tan(pi * wb)
     *
     * @note q is inverse of relative bandwidth (wc / wb)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setSOAPBP2(const float delta, const float gamma) {
      // Alternative implementation based on second order tunable all pass
      // delta = cos(2pi*wc) 
      mCoeffs.setSOAP2(delta, gamma);
        
      mW0 = -1.f;
      mD0 = 1.f;
        
      mW1 = 0.5f;
      mD1 = 0.f;
    }
      
    // -- All-Pass based Peak/Notch -------------
    /**
     * Calculate coefficients for second order all pass based peak/notch filter.
     *
     * @param   delta cos(2pi*wc)
     * @param   gamma tan(pi * wb)
     * @param   gain 10^(gain_db/20)
     *
     * @note q is inverse of relative bandwidth (wc / wb)
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setSOAPPN2(const float delta, const float gamma, const float gain) {
      // Alternative implementation based on second order tunable all pass
      // delta = cos(2pi*wc)

      const float h = gain - 1.f;
      const float g = (gain >=  1.f) ? 1.f : gain;
        
      const float c = (gamma - g) / (gamma + g);
      const float d = -delta;
        
      mCoeffs.ff0 = mCoeffs.fb2 = -c;
      mCoeffs.ff1 = mCoeffs.fb1 = d * (1.f - c);
      mCoeffs.ff2 = 1.f;
        
      mW0 = -1.f;
      mD0 = 1.f;

      mW1 = 0.5f * h;
      mD1 = 1.f;
    }

    BiQuad::Coeffs mCoeffs;
    float mD0, mD1, mW0, mW1;
    const uint32_t mVectorx4Loops;
    const uint32_t mVectorx2Loops;
    const uint32_t mVectorx1Loops;
    float mZ1[NumParallelFilters];
    float mZ2[NumParallelFilters];
  };
}
/** @} */
