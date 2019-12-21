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
 * @file    biquad.hpp
 * @brief   Generic biquad structure and convenience methods.
 *
 * @addtogroup dsp DSP
 * @{
 *
 */

#include "float_math.h"

/**
 * Common DSP Utilities
 */
namespace dsp {

  /**
   * Transposed form 2 Bi-Quad construct for FIR/IIR filters.
   */
  struct BiQuad {
    
    // Transposed Form 2
    
    /*=====================================================================*/
    /* Types and Data Structures.                                          */
    /*=====================================================================*/

    /**
     * Filter coefficients
     */
    typedef struct Coeffs {
      float ff0;
      float ff1;
      float ff2;
      float fb1;
      float fb2;
      
      /**
       * Default constructor
       */
      Coeffs() :
        ff0(0), ff1(0), ff2(0),
        fb1(0), fb2(0)
      { }

      // -- Pre-calculations -------------------

      /**
       * Convert Hz frequency to radians
       *
       * @param   fc Frequency in Hz
       * @param   fsrecip Reciprocal of sampling frequency (1/Fs)
       */
      static inline __attribute__((optimize("Ofast"),always_inline))
      float wc(const float fc, const float fsrecip) {
        return fc * fsrecip;
      }
      
      // -- Filter types -----------------------

      /**
       * Calculate coefficients for single pole low pass filter.
       *
       * @param   pole Pole position in radians
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setPoleLP(const float pole) {
        ff0 = 1.f - pole;
        fb1 = -pole;
        fb2 = ff2 = ff1 = 0.f;
      }

      /**
       * Calculate coefficients for single pole high pass filter.
       *
       * @param   pole Pole position in radians
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setPoleHP(const float pole) {
        ff0 = 1.f - pole;
        fb1 = pole;
        fb2 = ff2 = ff1 = 0.f;
      }

      /**
       * Calculate coefficients for single pole DC filter.
       *
       * @param   pole Pole position in radians
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setFODC(const float pole) {
        ff0 = 1.f;
        ff1 = -1.f;
        fb1 = -pole;
        fb2 = ff2 = 0.f;
      }

      /**
       * Calculate coefficients for first order low pass filter.
       *
       * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setFOLP(const float k) {
        const float kp1 = k+1.f;
        const float km1 = k-1.f;
        ff0 = ff1 = k / kp1;
        fb1 = km1 / kp1;
        fb2 = ff2 = 0.f;
      }

      /**
       * Calculate coefficients for first order high pass filter.
       *
       * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setFOHP(const float k) {
        // k = tan(pi*wc)
        const float kp1 = k+1.f;
        const float km1 = k-1.f;
        ff0 = 1.f / kp1;
        ff1 = -ff0;
        fb1 = km1 / kp1;
        fb2 = ff2 = 0.f;
      }

      /**
       * Calculate coefficients for first order all pass filter.
       *
       * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setFOAP(const float k) {
        // k = tan(pi*wc)
        const float kp1 = k+1.f;
        const float km1 = k-1.f;
        ff0 = fb1 = km1 / kp1;
        ff1 = 1.f;
        fb2 = ff2 = 0.f;
      }

      /**
       * Calculate coefficients for first order all pass filter.
       *
       * @param   wc cutoff frequency in radians
       *
       * @note Alternative implementation with no tangeant lookup
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setFOAP2(const float wc) {
        // Note: alternative implementation for use in phasers
        const float g1 = 1.f - wc;
        ff0 = g1;
        ff1 = -1;
        fb1 = -g1;
        fb2 = ff2 = 0.f;
      }

      /**
       * Calculate coefficients for second order DC filter.
       *
       * @param   pole Pole position in radians
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setSODC(const float pole) {
        ff0 = ff2 = 1.f;
        ff1 = 2.f;
        fb1 = -2.f * pole;
        fb2 = pole * pole;
      }

      /**
       * Calculate coefficients for second order low pass filter.
       *
       * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
       * @param   q Resonance with flat response at q = sqrt(2)
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setSOLP(const float k, const float q) {
        // k = tan(pi*wc)
        // flat response at q = sqrt(2)
        const float qk2 = q * k * k;
        const float qk2_k_q_r = 1.f / (qk2 + k + q);
        ff0 = ff2 = qk2 * qk2_k_q_r;
        ff1 = 2.f * ff0;
        fb1 = 2.f * (qk2 - q) * qk2_k_q_r;
        fb2 = (qk2 - k + q) * qk2_k_q_r;
      }

      /**
       * Calculate coefficients for second order high pass filter.
       *
       * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
       * @param   q Resonance with flat response at q = sqrt(2)
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setSOHP(const float k, const float q) {
        // k = tan(pi*wc)
        // flat response at q = sqrt(2)
        const float qk2 = q * k * k;
        const float qk2_k_q_r = 1.f / (qk2 + k + q);
        ff0 = ff2 = q * qk2_k_q_r;
        ff1 = -2.f * ff0;
        fb1 = 2.f * (qk2 - q) * qk2_k_q_r;
        fb2 = (qk2 - k + q) * qk2_k_q_r;
      }

      /**
       * Calculate coefficients for second order band pass filter.
       *
       * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
       * @param   q Resonance with flat response at q = sqrt(2)
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setSOBP(const float k, const float q) {
        // k = tan(pi*wc)
        // q is inverse of relative bandwidth (Fc / Fb)
        const float qk2 = q * k * k;
        const float qk2_k_q_r = 1.f / (qk2 + k + q);
        ff0 = k * qk2_k_q_r;
        ff1 = 0.f;
        ff2 = -ff0;
        fb1 = 2.f * (qk2 - q) * qk2_k_q_r;
        fb2 = (qk2 - k + q) * qk2_k_q_r;
      }

      /**
       * Calculate coefficients for second order band reject filter.
       *
       * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
       * @param   q Resonance with flat response at q = sqrt(2)
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setSOBR(const float k, const float q) {
        // k = tan(pi*wc)
        // q is inverse of relative bandwidth (Fc / Fb)
        const float qk2 = q * k * k;
        const float qk2_k_q_r = 1.f / (qk2 + k + q);
        ff0 = ff2 = (qk2 + q) * qk2_k_q_r;
        ff1 = fb1 = 2.f * (qk2 - q) * qk2_k_q_r;
        fb2 = (qk2 - k + q) * qk2_k_q_r;
      }

      /**
       * Calculate coefficients for second order all pass filter.
       *
       * @param   k Tangent of PI x cutoff frequency in radians: tan(pi*wc)
       * @param   q Inverse of relative bandwidth (Fc / Fb)
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setSOAP1(const float k, const float q) {
        // k = tan(pi*wc)
        // q is inverse of relative bandwidth (Fc / Fb)
        const float qk2 = q * k * k;
        const float qk2_k_q_r = 1.f / (qk2 + k + q);
        ff0 = fb2 = (qk2 - k + q) * qk2_k_q_r;
        ff1 = fb1 = 2.f * (qk2 - q) * qk2_k_q_r;
        ff2 = 1.f;
      }

      /**
       * Calculate coefficients for second order all pass filter.
       *
       * @param   delta cos(2pi*wc)
       * @param   gamma tan(pi * wb)
       *
       * @note q is inverse of relative bandwidth (wc / wb)
       * @note Alternative implementation, so called "tunable" in DAFX second edition.
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setSOAP2(const float delta, const float gamma) {
        // Note: Alternative implementation .. so called "tunable" in DAFX.
        // delta = cos(2pi*wc)
        const float c = (gamma - 1.f) / (gamma + 1.f);
        const float d = -delta;
        ff0 = fb2 = -c;
        ff1 = fb1 = d * (1.f - c);
        ff2 = 1.f;
      }

      /**
       * Calculate coefficients for second order all pass filter.
       *
       * @param   delta cos(2pi*wc)
       * @param   radius 
       *
       * @note Another alternative implementation.
       */
      inline __attribute__((optimize("Ofast"),always_inline))
      void setSOAP3(const float delta, const float radius) {
        // Note: alternative implementation for use in phasers
        // delta = cos(2pi * wc)
        const float a1 = -2.f * radius * delta;
        const float a2 = radius * radius;
        ff0 = fb2 = a2;
        ff1 = fb1 = a1;
        ff2 = 1.f;
      }
        
    } Coeffs;
      
    /*=====================================================================*/
    /* Constructor / Destructor.                                           */
    /*=====================================================================*/

    /**
     * Default constructor
     */
    BiQuad(void) : mZ1(0), mZ2(0)
    { }
      
    /*=====================================================================*/
    /* Public Methods.                                                     */
    /*=====================================================================*/

    /**
     * Flush internal delays
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void flush(void) {
      mZ1 = mZ2 = 0;
    }

    /**
     * Second order processing of one sample
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float process_so(const float xn) {
      float acc = mCoeffs.ff0 * xn + mZ1;
      mZ1 = mCoeffs.ff1 * xn + mZ2;
      mZ2 = mCoeffs.ff2 * xn;
      mZ1 -= mCoeffs.fb1 * acc;
      mZ2 -= mCoeffs.fb2 * acc;
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
    float process_fo(const float xn) {
      float acc = mCoeffs.ff0 * xn + mZ1;
      mZ1 = mCoeffs.ff1 * xn;
      mZ1 -= mCoeffs.fb1 * acc;
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
      return process_so(xn);
    }
      
    /*=====================================================================*/
    /* Member Variables.                                                   */
    /*=====================================================================*/

    /** Coefficients for the Bi-Quad construct */
    Coeffs mCoeffs;
    float mZ1, mZ2;      
  };

  /**
   * Extended transposed form 2 Bi-Quad construct
   */
  struct ExtBiQuad {
    // Extended BiQuad structure
      
    /*=====================================================================*/
    /* Types and Data Structures.                                          */
    /*=====================================================================*/
            
    /*=====================================================================*/
    /* Constructor / Destructor.                                           */
    /*=====================================================================*/

    /**
     * Default constructor.
     */
    ExtBiQuad(void) :
      mZ1(0), mZ2(0),
      mD0(0), mD1(0),
      mW0(0), mW1(0)
    { }
      
    /*=====================================================================*/
    /* Public Methods.                                                     */
    /*=====================================================================*/

    /**
     * Flush internal delays
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    void flush(void) {
      mZ1 = mZ2 = 0;
    }

    /**
     * Second order processing of one sample
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float process_so(const float xn) {
      float acc = mCoeffs.ff0 * xn + mZ1;
      mZ1 = mCoeffs.ff1 * xn + mZ2;
      mZ2 = mCoeffs.ff2 * xn;
      mZ1 -= mCoeffs.fb1 * acc;
      mZ2 -= mCoeffs.fb2 * acc;
      return mW1 * (mW0 * acc + mD0 * xn) + mD1 * xn;
    }

    /**
     * First order processing of one sample
     *
     * @param xn  Input sample
     *
     * @return Output sample
     */
    inline __attribute__((optimize("Ofast"),always_inline))
    float process_fo(const float xn) {
      float acc = mCoeffs.ff0 * xn + mZ1;
      mZ1 = mCoeffs.ff1 * xn;
      mZ1 -= mCoeffs.fb1 * acc;
      return mW1 * (mW0 * acc + mD0 * xn) + mD1 * xn;
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
      return process_so(xn);
    }

    // -- Invertable All-Pass based Low/High Pass -------

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
      
    /*=====================================================================*/
    /* Member Variables.                                                   */
    /*=====================================================================*/

    /** Coefficients for the Bi-Quad construct */
    BiQuad::Coeffs mCoeffs;
    float mD0, mD1, mW0, mW1;
    float mZ1, mZ2;
  };    
}

/** @} */
