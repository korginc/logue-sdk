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

/**
 * @file    delayline.hpp
 * @brief   Basic Delay Lines.
 *
 * @addtogroup dsp DSP
 * @{
 *
 */

#include "float_math.h"
#include "int_math.h"
#include "buffer_ops.h"

/**
 * Common DSP Utilities
 */
namespace dsp {

  /**
   * Basic delay line abstraction.
   */
  struct DelayLine {
      
    /*===========================================================================*/
    /* Types and Data Structures.                                                */
    /*===========================================================================*/
      
    /*===========================================================================*/
    /* Constructor / Destructor.                                                 */
    /*===========================================================================*/

    /**
     * Default constructor
     */
    DelayLine(void) :
      mLine(0),
      mFracZ(0),
      mSize(0),
      mMask(0),
      mWriteIdx(0)
    { }

    /**
     * Constructor with explicit memory area to use as backing buffer for delay line.
     *
     * @param ram Pointer to memory buffer
     * @param line_size Size in float of memory buffer
     *
     */
    DelayLine(float *ram, size_t line_size) :
      mLine(ram),
      mFracZ(0),
      mSize(line_size),
      mMask(line_size-1),
      mWriteIdx(0)
    { }
      
    /*===========================================================================*/
    /* Public Methods.                                                           */
    /*===========================================================================*/

    /**
     * Zero clear the whole delay line.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void clear(void) {
      buf_clr_f32((float *)mLine, mSize);
    }

    /**
     * Set the memory area to use as backing buffer for the delay line.
     *
     * @param ram Pointer to memory buffer
     * @param line_size Size in float of memory buffer
     *
     * @note Will round size to next power of two.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setMemory(float *ram, size_t line_size) {
      mLine = ram;
      mSize = nextpow2_u32(line_size); // must be power of 2
      mMask = (mSize-1);
      mWriteIdx = 0;
    }

    /**
     * Write a single sample to the head of the delay line
     *
     * @param s Sample to write
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void write(const float s) {
      mLine[(mWriteIdx--) & mMask] = s;
    }

    /**
     * Read a single sample from the delay line at given position from current write index.
     *
     * @param pos Offset from write index
     * @return Sample at given position from write index
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float read(const uint32_t pos) {
      return mLine[(mWriteIdx + pos) & mMask];
    }

    /**
     * Read a sample from the delay line at a fractional position from current write index.
     *
     * @param pos Offset from write index as floating point.
     * @return Interpolated sample at given fractional position from write index
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float readFrac(const float pos) {
      const uint32_t base = (uint32_t)pos;
      const float frac = pos - base;
      const float s0 = read(base);
      const float s1 = read(base+1);
      return linintf(frac, s0, s1);
    }

    /**
     * Read a sample from the delay line at a position from current write index with interpolation from last read.
     *
     * @param pos Offset from write index
     * @param frac Interpolation from last read pair.
     * @return Interpolation of last read sample and sample at given position from write index.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float readFracz(const uint32_t pos, const float frac) {
      const float s0 = read(pos);
      const float y = linintf(frac, s0, mFracZ);
      mFracZ = s0;
      return y;
    }
      
      
    /*===========================================================================*/
    /* Member Variables.                                                         */
    /*===========================================================================*/
      
    float   *mLine;
    float    mFracZ;
    size_t   mSize;
    size_t   mMask;
    uint32_t mWriteIdx;
      
  };

  /**
   * Dual channel delay line abstraction with interleaved samples. 
   */
  struct DualDelayLine {
      
    /*===========================================================================*/
    /* Types and Data Structures.                                                */
    /*===========================================================================*/
      
    /*===========================================================================*/
    /* Constructor / Destructor.                                                 */
    /*===========================================================================*/

    /**
     * Default constructor.
     */
    DualDelayLine(void) :
      mLine(0),
      mSize(0),
      mMask(0),
      mWriteIdx(0)
    { }

    
    /**
     * Constructor with explicit memory area to use as backing buffer for delay line.
     *
     * @param ram Pointer to memory buffer
     * @param line_size Size in float pairs of memory buffer
     *
     */
    DualDelayLine(f32pair_t *ram, size_t line_size) :
      mWriteIdx(0)
    {
      setMemory(ram, line_size);
    }
      
    /*===========================================================================*/
    /* Public Methods.                                                           */
    /*===========================================================================*/

    /**
     * Zero clear the whole delay line.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void clear(void) {
      buf_clr_f32((float *)mLine, 2*mSize);
    }

    /**
     * Set the memory area to use as backing buffer for the delay line.
     *
     * @param ram Pointer to memory buffer
     * @param line_size Size in float pairs of memory buffer
     *
     * @note Will round size to next power of two.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void setMemory(f32pair_t *ram, size_t line_size) {
      mLine = ram;
      mSize = nextpow2_u32(line_size); // must be power of 2
      mMask = (mSize-1);
      mWriteIdx = 0;
    }

    /**
     * Write a sample pair to the delay line
     *
     * @param p Reference to float pair.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void write(const f32pair_t &p) {
      mLine[(mWriteIdx--) & mMask] = p;
    }

    /**
     * Read a sample pair from the delay line at given position from current write index.
     *
     * @param pos Offset from write index
     * @return Sample pair at given position from write index
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    f32pair_t read(const uint32_t pos) {
      return mLine[(mWriteIdx + pos) & mMask];
    }

    /**
     * Read a sample pair from the delay line at a fractional position from current write index.
     *
     * @param pos Offset from write index as floating point.
     * @return Interpolated sample pair at given fractional position from write index
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    f32pair_t readFrac(const float pos) {
      const int32_t base = (uint32_t)pos;
      const float frac = pos - base;
      const f32pair_t p0 = read(base);
      const f32pair_t p1 = read(base+1);
        
      return f32pair_linint(frac, p0, p1); 
    }

    /**
     * Read a sample pair from the delay line at a position from current write index with interpolation from last read.
     *
     * @param pos Offset from write index
     * @param frac Interpolation from last read pair.
     * @return Interpolation of last read sample pair and sample pair at given position from write index.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    f32pair_t readFracz(const uint32_t pos, const float frac) {
      const f32pair_t p0 = read(pos);
      const f32pair_t y = f32pair_linint(frac, p0, mFracZ);
      mFracZ = p0;
      return y;
    }

    /**
     * Read a single sample from the delay line's primary channel at given position from current write index.
     *
     * @param pos Offset from write index.
     * @return Sample at given position from write index
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float read0(const uint32_t pos) {
      return (mLine[(mWriteIdx + pos) & mMask]).a;
    }

    /**
     * Read a single sample from the delay line's secondary channel at given position from current write index.
     *
     * @param pos Offset from write index.
     * @return Sample at given position from write index
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float read1(const uint32_t pos) {
      return (mLine[(mWriteIdx + pos) & mMask]).b;
    }

    /**
     * Read a single sample from the delay line's primary channel at a fractional position from current write index.
     *
     * @param pos Offset from write index as floating point.
     * @return Interpolated sample at given fractional position from write index
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float read0Frac(const float pos) {
      const int32_t base = (uint32_t)pos;
      const float frac = pos - base;
      const float f0 = read0(base);
      const float f1 = read0(base+1);
      return linintf(frac, f0, f1);
    }

    /**
     * Read a single sample from the delay line's primary channel at a position from current write index with interpolation from last read.
     *
     * @param pos Offset from write index
     * @param frac Interpolation from last read pair.
     * @return Interpolation of last read sample and sample at given position from write index.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float read0Fracz(const uint32_t pos, const float frac) {
      const float f0 = read0(pos);
      const float y = linintf(frac, f0, mFracZ.a);
      mFracZ.a = f0;
      return y;
    }

    /**
     * Read a single sample from the delay line's secondary channel at a fractional position from current write index.
     *
     * @param pos Offset from write index as floating point.
     * @return Interpolated sample at given fractional position from write index
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float read1Frac(const float pos) {
      const int32_t base = (uint32_t)pos;
      const float frac = pos - base;
      const float f0 = read1(base);
      const float f1 = read1(base+1);
      return linintf(frac, f0, f1);
    }

    /**
     * Read a single sample from the delay line's secondary channel at a position from current write index with interpolation from last read.
     *
     * @param pos Offset from write index
     * @param frac Interpolation from last read pair.
     * @return Interpolation of last read sample and sample at given position from write index.
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float read1Fracz(const uint32_t pos, const float frac) {
      const float f0 = read1(pos);
      const float y = linintf(frac, f0, mFracZ.b);
      mFracZ.b = f0;
      return y;
    }
      
    /*===========================================================================*/
    /* Member Variables.                                                         */
    /*===========================================================================*/
      
    f32pair_t *mLine;
    f32pair_t  mFracZ;
    size_t     mSize;
    size_t     mMask;
    uint32_t   mWriteIdx;
      
  };
    
    
}

/** @} */

