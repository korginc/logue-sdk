#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2023, KORG INC.
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

/*
 *  File: modfx.h
 *
 *  Dummy modfx effect template instance.
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <iostream>

#include "utils/buffer_ops.h" // for buf_clr_f32()
#include "utils/int_math.h"   // for clipminmaxi32()
#include "utils/mk2_utils.h"
#include "utils/io_ops.h"
#include "runtime.h"
#include "unit_modfx.h"
#include "macros.h"
#include "dsp/simplelfo.hpp"
#include "dsp/LinearSmoother.h"
#include "dsp/mk2_biquad.hpp"
#include "dsp/delayline.hpp"

class Vibrato 
{
 public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  enum 
  {
    BUFFER_LENGTH = MODFX_MEMORY_SIZE,
  };

  enum 
  {
    kParamRate = 0U,
    kParamDepth,
    kParamSpread,
    kParamLowCut,
    kParamShape,
    kParamTempoSync,
    kParamDeepFry,
    kNumParams
  };

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Vibrato(void):
  mSineMix(0),
  mSineSquareMix(0),
  mTriMix(0),
  mLfoPhaseOffset(0),
  mCompressorGain(1),
  mOutputGain(1),
  mOutputCompensationGain(1),
  mThreshold(0),
  mRatio(0),
  mAllocatedBuffer(nullptr),
  mTempo(120),
  mMinDelayMS(0.04f),
  mMaxDelayMS(20.f),
  mMinLowCutFreq(20.f),
  mMaxLowCutFreq(1000.f),
  mMinLfoFreq(0.01),
  mBreakpointLfoFreq(20.f),
  mMaxLfoFreq(300.f),
  mLfoSmoothingCoeff(0.01),
  mDeepFryGain(4000.f),
  mDeepFryCompensation(0.25f),
  mRmsCoeff(0.0457284588),
  mAttack(0.04479885088), // 1 ms
  mRelease(0.00045822831) // 100 ms
  {

  }

  ~Vibrato(void) 
  {} // Note: will never actually be called for statically allocated instances

  inline int8_t Init(const unit_runtime_desc_t * desc) 
  {
    if (!desc)
      return k_unit_err_undef;
    
    // Note: make sure the unit is being loaded to the correct platform/module target
    if (desc->target != unit_header.target)
      return k_unit_err_target;
    
    // Note: check API compatibility with the one this unit was built against
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;
    
    // Check compatibility of samplerate with unit, for microkorg2 should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    if (desc->input_channels != 2 || desc->output_channels != 2)  // should be stereo input/output
      return k_unit_err_geometry;

    // If SDRAM buffers are required they must be allocated here
    if (!desc->hooks.sdram_alloc)
      return k_unit_err_memory;
    float *m = (float *)desc->hooks.sdram_alloc(BUFFER_LENGTH * sizeof(float));
    if (!m)
      return k_unit_err_memory;

    // microkorg2 handles buffer clearing
    // buf_clr_f32(m, BUFFER_LENGTH);

    mAllocatedBuffer = m;
    
    // Cache the runtime descriptor for later use
    mRuntimeDesc = *desc;

    buf_clr_u32(reinterpret_cast<uint32_t *>(mParams), kNumParams);
    
    Reset();

    const float sixtySeconds = 60.f;
    mSyncTable[kSync1_64] = sixtySeconds * 1.f / 16.f;
    mSyncTable[kSync1_32] = sixtySeconds * 1.f / 8.f;
    mSyncTable[kSync1_24] = sixtySeconds * 1.f / 6.f;
    mSyncTable[kSync1_16] = sixtySeconds * 1.f / 4.f;
    mSyncTable[kSync1_12] = sixtySeconds * 1.f / 3.f;
    mSyncTable[kSync3_32] = sixtySeconds * 3.f / 8.f;
    mSyncTable[kSync1_8 ] = sixtySeconds * 1.f / 2.f;
    mSyncTable[kSync1_6 ] = sixtySeconds * 2.f / 3.f;
    mSyncTable[kSync3_16] = sixtySeconds * 3.f / 4.f;
    mSyncTable[kSync1_4 ] = sixtySeconds * 1.f / 1.f;
    mSyncTable[kSync1_3 ] = sixtySeconds * 4.f / 3.f;
    mSyncTable[kSync3_8 ] = sixtySeconds * 3.f / 2.f;
    mSyncTable[kSync1_2 ] = sixtySeconds * 2.f / 1.f;
    mSyncTable[kSync3_4 ] = sixtySeconds * 3.f / 1.f;
    mSyncTable[kSync1_1 ] = sixtySeconds * 4.f / 1.f;

    return k_unit_err_none;
  }

  inline void Teardown()
  {
    // Note: buffers allocated via sdram_alloc are automatically freed after unit teardown
    // Note: cleanup and release resources if any
    mAllocatedBuffer = nullptr;
  }

  inline void Reset() 
  {
    // Note: Reset effect state, excluding exposed parameter values.
    mLowCutFilter.flush();
    mBassBoostFilter.flush();
    mLfo.reset();
    mDepthSmoother.Flush();
    mDepthSmoother.SetInterval(1.f / (mRuntimeDesc.frames_per_buffer * 16.f));

    // clear handled by microkorg2 system
    mDelayLine.setMemory(reinterpret_cast<f32pair_t *>(mAllocatedBuffer), BUFFER_LENGTH >> 1);

    mRmsZ[0] = 0;
    mRmsZ[1] = 0;
    mEnvZ[0] = 0;
    mEnvZ[1] = 0;
    mLfoZ[0] = 0;
    mLfoZ[1] = 0;
  }

  inline void Resume() 
  {
    // Note: Effect will resume and exit suspend state. Usually means the synth
    // was selected and the render callback will be called again

    // Note: If it is required to clear large memory buffers, consider setting a flag
    //       and trigger an asynchronous progressive clear on the audio thread (Process() handler)
    Reset();
  }

  inline void Suspend() 
  {
    // Note: Effect will enter suspend state. Usually means another effect was
    // selected and thus the render callback will not be called
  }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void Process(const float * in, float * out, size_t frames) 
  {
    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1); 

    UpdateParameters();

    const float samplerate = mRuntimeDesc.samplerate * 0.001;
    for (; out_p != out_e; in_p += 2, out_p += 2)
    {
      f32pair_t dry = {in_p[0], in_p[1]};

      // write to delay line
      mDelayLine.write(dry);
      
      // calculate delay time
      mLfo.cycle();
      float32x2_t sine = float32x2(mLfo.sine_uni(), mLfo.sine_uni_off(mLfoPhaseOffset));
      float32x2_t sineSquare = float32x2_mul(sine, sine);
      float32x2_t tri = float32x2(mLfo.triangle_uni(), mLfo.triangle_uni_off(mLfoPhaseOffset));
      float32x2_t lfo = float32x2_mulscal(sine, mSineMix);
      lfo = float32x2_fmulscaladd(lfo, sineSquare, mSineSquareMix);
      lfo = float32x2_fmulscaladd(lfo, tri, mTriMix);
      
      // smooth to avoid noise from spread param
      float32x2_t lfoZx2 = f32x2_ld(mLfoZ);
      f32x2_str(mLfoZ, float32x2_fmulscaladd(lfoZx2, float32x2_sub(lfo, lfoZx2), mLfoSmoothingCoeff));
      
      float depth = mDepthSmoother.Process();
      float delayTimeL = (mMinDelayMS + depth * mLfoZ[0]) * samplerate;
      float delayTimeR = (mMinDelayMS + depth * mLfoZ[1]) * samplerate;

      // read delay output
      float delayOutL = mDelayLine.read0Frac(delayTimeL);
      float delayOutR = mDelayLine.read1Frac(delayTimeR);

      out_p[0] = delayOutL;
      out_p[1] = delayOutR;

      float32x2_t lowCutOut = mLowCutFilter.process_fo_x2(float32x2(delayOutL, delayOutR));

      // compressor
      float32x2_t compressorOut = ProcessCompressor(float32x2_mulscal(lowCutOut, mCompressorGain));
      compressorOut = float32x2_mulscal(compressorOut, mOutputGain);
      compressorOut = clipminmaxfx2(f32x2_dup(-0.5f), compressorOut, f32x2_dup(0.5f));
      compressorOut = float32x2_mulscal(compressorOut, mOutputCompensationGain);

      // bass boost
      float32x2_t bassBoostOut = mBassBoostFilter.process_fo_x2(compressorOut);

      f32x2_str(out_p, bassBoostOut);
    }
  }

  inline void setParameter(uint8_t index, int32_t value) 
  {
    mParams[index] = value;
  }

  inline int32_t getParameterValue(uint8_t index) const
  {
    return mParams[index];
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const 
  {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue
    switch (index)
    {
      case kParamRate:
      {
        static const char * rateStrings[kNumSync] = 
        {
          "1/64",
          "1/32",
          "1/24",
          "1/16",
          "1/12",
          "3/32",
          "1/8",
          "1/6",
          "3/16",
          "1/4",
          "1/3",
          "3/8",
          "1/2",
          "3/4",        
          "1/1",
        };
    
        if(mParams[kParamTempoSync])
        {
          float rate = param_10bit_to_f32(value);
          int index = quantizeNormalizedValueToRange(1.f - rate, 0, kNumSync - 1);
          return rateStrings[index];
        }
        break;
      }
      
      default:
        break;
    }
    return nullptr;
  }

  inline void setTempo(uint32_t tempo) 
  {
    mTempo = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
  }

  inline void tempo4ppqnTick(uint32_t counter) 
  {
    (void)counter;
  }

  /*===========================================================================*/
  /* Static Members. */
  /*===========================================================================*/
  
 private:
  /*===========================================================================*/
  /* Private Member Variables. */
  /*===========================================================================*/

  enum
  {
    kSync1_64,
    kSync1_32,
    kSync1_24,
    kSync1_16,
    kSync1_12,
    kSync3_32,
    kSync1_8,
    kSync1_6,
    kSync3_16,
    kSync1_4,
    kSync1_3,
    kSync3_8,
    kSync1_2,
    kSync3_4,
    kSync1_1,
    kNumSync
  };

  dsp::ParallelBiQuad<2> mLowCutFilter;
  dsp::ParallelExtBiQuad<2> mBassBoostFilter;
  dsp::SimpleLFO mLfo;
  dsp::LinearSmoother mDepthSmoother;
  dsp::DualDelayLine mDelayLine;

  float mSineMix;
  float mSineSquareMix;
  float mTriMix;
  float mLfoPhaseOffset;
  float mCompressorGain;
  float mOutputGain;
  float mOutputCompensationGain;
  float mLfoZ[2];

  // simple compressor
  float mThreshold;
  float mRatio;
  float mRmsZ[2];
  float mEnvZ[2];
  float mSyncTable[kNumSync];

  std::atomic_uint_fast32_t mFlags;
  unit_runtime_desc_t mRuntimeDesc;
  float * mAllocatedBuffer;
  float mTempo;

  int32_t mParams[kNumParams];
  
  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  fast_inline void UpdateParameters()
  {
    const float fsRecip = 1.f / mRuntimeDesc.samplerate;

    // calculate shape mix
    const float numCrossfades = 2;
    float shape = param_10bit_to_f32(mParams[kParamShape]) * numCrossfades;
    mSineMix = 1.f - clip01f(shape);
    mSineSquareMix = clip01f(1.f - si_fabsf(1.f - shape));
    mTriMix = clip01f(1.f - si_fabsf(2.f - shape));

    // lfo
    mLfoPhaseOffset = mParams[kParamSpread] * 0.01;

    float lfoFreq = param_10bit_to_f32(mParams[kParamRate]);
    if(mParams[kParamTempoSync])
    {
      int syncIndex = quantizeNormalizedValueToRange(1.f - lfoFreq, 0, kNumSync - 1);
      lfoFreq = mTempo / mSyncTable[syncIndex];
    }
    else
    {
      float lfoFreqCurve = lfoFreq < 0.75 ? lfoFreq * (1.f / 0.75f) : (lfoFreq - 0.75f) * (1.f / (1.f - 0.75));
      lfoFreq = (lfoFreq < 0.75) ? 
                  scaleNormalizedValueToRange(lfoFreqCurve * lfoFreqCurve, mMinLfoFreq, mBreakpointLfoFreq) : 
                  scaleNormalizedValueToRange(lfoFreqCurve, mBreakpointLfoFreq, mMaxLfoFreq);
    }
    mLfo.setF0(lfoFreq, fsRecip);

    // minimum delay is used as an offset, so calculate depth from zero
    float depth = param_10bit_to_f32(mParams[kParamDepth]);
    depth = scaleNormalizedValueToRange(depth, 0.f, mMaxDelayMS - mMinDelayMS);
    mDepthSmoother.SetTarget(depth);

    // filtering
    float lowCutNormalized = param_10bit_to_f32(mParams[kParamLowCut]);
    const float lowCutFreq = scaleNormalizedValueToRange(lowCutNormalized * lowCutNormalized, mMinLowCutFreq, mMaxLowCutFreq);
    const float lowCutWc = dsp::BiQuad::Coeffs::wc(lowCutFreq, fsRecip);
    mLowCutFilter.mCoeffs.setFOHP(dsp::BiQuad::Coeffs::tanPiWc(lowCutWc));

    const float bassFc = 100.f;
    float bassGain = 1.f; // amplitude, not dB
    if(mParams[kParamDeepFry])
    {
      // Squash the output
      mRatio = 1.f;
      mThreshold = -60.f;

      mCompressorGain = 2.f;
      mOutputGain = mDeepFryGain;
      mOutputCompensationGain = mDeepFryCompensation;
      bassGain = 4.f; // roughly 12dB
    }
    else
    {
      // bypass
      mRatio = 0.f;
      mThreshold = 0.f;

      mCompressorGain = 1.f;
      mOutputGain = 1.f;
      mOutputCompensationGain = 1.f;
    }
    const float bassBoostWcL = dsp::BiQuad::Coeffs::wc(bassFc, fsRecip);
    const float bassBoostKL = dsp::BiQuad::Coeffs::tanPiWc(bassBoostWcL);
    mBassBoostFilter.setFOLS(bassBoostKL, bassGain);
  }

  float32x2_t fast_inline ProcessCompressor(float32x2_t x)
  {
    float32x2_t rmsZ = float32x2_mulscal(f32x2_ld(mRmsZ), (1.f - mRmsCoeff));
    f32x2_str(mRmsZ, float32x2_fmulscaladd(rmsZ, float32x2_mul(x, x), mRmsCoeff));

    float32x2_t control = {fasterlogf(mRmsZ[0]), fasterlogf(mRmsZ[1])};
    control = float32x2_mulscal(control, 10.f);
    control = clipminfx2(control, f32x2_dup(-100.f));
    control = float32x2_mulscal(float32x2_subscal(control, mThreshold), mRatio);
    control = clipminfx2(control, f32x2_dup(0.f));
    control = float32x2_mulscal(control, 0.05); // divide control / 20 for amplitude calculation 10^(x/20)

    float controlAmp[2];
    f32x2_str(controlAmp, control);
    controlAmp[0] = fasterpowf(10.f, -controlAmp[0]);
    controlAmp[1] = fasterpowf(10.f, -controlAmp[1]);

    control = f32x2_ld(controlAmp);
    float32x2_t envZ = f32x2_ld(mEnvZ);
    float32x2_t coeff = float32x2_sel(float32x2_lt(control, envZ), f32x2_dup(mAttack), f32x2_dup(mRelease));
    envZ = float32x2_fmuladd(float32x2_mul(control, coeff), 
                             float32x2_sub(f32x2_dup(1.f), coeff), envZ);
    f32x2_str(mEnvZ, envZ);
    return float32x2_mul(x, envZ);
  }
  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/

  const float mMinDelayMS;
  const float mMaxDelayMS;
  const float mMinLowCutFreq;
  const float mMaxLowCutFreq;
  const float mMinLfoFreq;
  const float mBreakpointLfoFreq;
  const float mMaxLfoFreq;
  const float mLfoSmoothingCoeff;
  const float mDeepFryGain;
  const float mDeepFryCompensation;
  const float mRmsCoeff;
  const float mAttack;
  const float mRelease;
};
