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
 *  File: delay.h
 *
 *  Dummy delay effect template instance.
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
#include "runtime.h"
#include "unit_delfx.h"
#include "macros.h"
#include "dsp/LinearSmoother.h"
#include "dsp/delayline.hpp"
#include "dsp/mk2_biquad.hpp"

class MultitapDelay {
 public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  enum {
    BUFFER_LENGTH = DELFX_MEMORY_SIZE,
  };

  enum {
    kParamDelayTime = 0U,
    kParamFeedback,
    kParamTone, 
    kParamInputMix,
    kParamSpread,
    kParamTapTimeScale,
    kParamResonance,
    kParamWet,
    kNumParams
  };
  
  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  MultitapDelay(void):
  mTempo(120),
  allocated_buffer_(nullptr),
  mDelayTimeSmoothingCoeff(0.005)
  {

  }

  ~MultitapDelay(void) 
  {} // Note: will never actually be called for statically allocated instances

  inline int8_t Init(const unit_runtime_desc_t * desc) {
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

    allocated_buffer_ = m;
    
    // Cache the runtime descriptor for later use
    runtime_desc_ = *desc;

    buf_clr_u32(reinterpret_cast<uint32_t *>(params_), kNumParams);
    
    return k_unit_err_none;
  }

  inline void Teardown() {
    // Note: buffers allocated via sdram_alloc are automatically freed after unit teardown
    // Note: cleanup and release resources if any
    allocated_buffer_ = nullptr;
  }

  inline void Reset() {
    // Note: Reset effect state, excluding exposed parameter values.
    float * delayLine = allocated_buffer_;

    mDelayTimeRange = 1.f / float((unit_header.params[kParamDelayTime].max - unit_header.params[kParamDelayTime].min));

    const uint32_t delayLineSize = BUFFER_LENGTH >> 1;

    mDelayLine1.setMemory(delayLine, delayLineSize);
    delayLine += delayLineSize;
    mDelayLine2.setMemory(delayLine, delayLineSize);

    mMixSmoother.SetTarget(params_[kParamWet] * 0.01);
    mInputSpreadSmoother.SetTarget(params_[kParamInputMix] * 0.01);
    mOutputSpreadSmoother.SetTarget(params_[kParamSpread] * 0.01);
    mFilterMixSmoother.SetTarget(params_[kParamTone] < 0);

    mMixSmoother.Flush();
    mInputSpreadSmoother.Flush();
    mOutputSpreadSmoother.Flush();
    mFilterMixSmoother.Flush();
    mCutoffZ = params_[kParamTone] * 0.01;

    mDelayTimeZ[kTap1] = mDelayTime[kTap1];
    mDelayTimeZ[kTap2] = mDelayTime[kTap2];
    mDelayTimeZ[kTap3] = mDelayTime[kTap3];
    mDelayTimeZ[kTap4] = mDelayTime[kTap4];

    mOutputFilters.flush();

    CookFilterCoeffs();

    mPrimaryFeedbackSmoother.SetTarget(CalculatePrimaryFeedback(CalculateFeedback(mDelayTime[kTap2] * mDelayTimeRange)));
    mSecondaryFeedbackSmoother.SetTarget(CalculateSecondaryFeedback(CalculateFeedback(mDelayTime[kTap3] * mDelayTimeRange)));
    mPrimaryFeedbackSmoother.Flush();
    mSecondaryFeedbackSmoother.Flush();
  }

  inline void Resume() {
    Reset();
  }

  inline void Suspend() {
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
    const float * out_e = out_p + (frames << 1);  // assuming stereo output

    UpdateParameters();

    const float feedbackScale = 0.5325f;
    float wetSig[4];
    float32x4_t delayTimeZ = f32x4_ld(mDelayTimeZ);
    const float32x4_t delayTimeTarget = f32x4_ld(mDelayTime);
    for (; out_p != out_e; in_p += 2, out_p += 2) 
    {      
      const float dryL = in_p[0];
      const float dryR = in_p[1];

      delayTimeZ = float32x4_add(delayTimeZ, float32x4_mulscal(float32x4_sub(delayTimeTarget, delayTimeZ), mDelayTimeSmoothingCoeff));
      f32x4_str(mDelayTimeZ, delayTimeZ);
      const float tap1 = mDelayLine1.readFrac(mDelayTimeZ[kTap1]);
      const float tap2 = mDelayLine2.readFrac(mDelayTimeZ[kTap2]);
      const float tap3 = mDelayLine1.readFrac(mDelayTimeZ[kTap3]);
      const float tap4 = mDelayLine2.readFrac(mDelayTimeZ[kTap4]);
      
      const float inputSpreadMix = mInputSpreadSmoother.Process();
      const float primaryFeedback = mPrimaryFeedbackSmoother.Process();
      const float secondaryFeedback = mSecondaryFeedbackSmoother.Process();
      float tap1Fb = tap1 * primaryFeedback;
      float tap2Fb = tap2 * (primaryFeedback * inputSpreadMix + secondaryFeedback * (1.f - inputSpreadMix));
      float tap3Fb = tap3 * secondaryFeedback;
      float tap4Fb = tap4 * secondaryFeedback;

      float fb1 = (tap1Fb + tap3Fb) * inputSpreadMix;
      fb1 = (fb1 + (tap2Fb + tap4Fb) * (1.f - inputSpreadMix));
      fb1 = clipminmaxf(-1.f, fb1 * feedbackScale, 1.f);

      float fb2 = tap2Fb + tap4Fb;
      fb2 = (fb2 + (tap1Fb + tap3Fb) * (1.f - inputSpreadMix));
      fb2 = clipminmaxf(-1.f, fb2 * (feedbackScale * inputSpreadMix), 1.f);

      const float delay1LeftMix = (0.5f + 0.5f * inputSpreadMix);
      const float delay1RightMix = (0.5f - 0.5f * inputSpreadMix);
      const float delay1OutputMix = 1.f - inputSpreadMix;
      const float delay2RightMix = inputSpreadMix;
      const float delay1In = dryL * delay1LeftMix + dryR * delay1RightMix + fb1;
      const float delay2In = tap1 * delay1OutputMix + dryR * delay2RightMix + fb2;

      mDelayLine1.write(delay1In);
      mDelayLine2.write(delay2In);
      
      const float outputSpread = mOutputSpreadSmoother.Process();
      const float outputSpreadFast = clipmaxf(outputSpread * 1.5f, 1.f);
      const float primaryTapLevel = (0.3 + 0.45 * outputSpread);
      const float primaryTapLevelFast = (0.3 + 0.45 * outputSpreadFast);
      const float secondaryTapLevel = (0.3 - (outputSpread) * 0.3f);
      const float secondaryTapLevelFast = (0.3 - (outputSpreadFast) * 0.3f);
      
      wetSig[0] = tap1 * primaryTapLevel;
      wetSig[0] += tap2 * secondaryTapLevel;
      wetSig[0] += tap3 * primaryTapLevelFast;
      wetSig[0] += tap4 * secondaryTapLevelFast;
      wetSig[2] = wetSig[0];

      wetSig[1] = tap1 * secondaryTapLevel;
      wetSig[1] += tap2 * primaryTapLevel;
      wetSig[1] += tap3 * secondaryTapLevelFast;
      wetSig[1] += tap4 * primaryTapLevelFast;
      wetSig[3] = wetSig[1];

      float32x4_t filterOut = mOutputFilters.process_so_x4(f32x4_ld(out), mOutputFilterCoeffs, 0);
      const float lpfMix = mFilterMixSmoother.Process();
      const float hpfMix = 1.f - lpfMix;
      filterOut = float32x4_mul(filterOut, float32x4(lpfMix, lpfMix, hpfMix, hpfMix));
      float32x2_t wetSigx2 = float32x2_add(float32x4_high(filterOut), float32x4_low(filterOut));

      const float wet = mMixSmoother.Process();
      const float dry = (1.f - si_fabsf(wet));
      f32x2_str(out_p, float32x2_add(float32x2_mulscal(wetSigx2, wet), float32x2_mulscal(float32x2(dryL, dryR), dry)));
    }
  }

  inline void setParameter(uint8_t index, int32_t value) 
  {
    params_[index] = value;
  }

  inline int32_t getParameterValue(uint8_t index) const
  {
    return params_[index];
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t /*value*/) const {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue
    
    switch (index) {
    default:
      break;
    }
    
    return nullptr;
  }

  inline void setTempo(uint32_t tempo) {
    mTempo = (tempo >> 16) + (tempo & 0xFFFF) / static_cast<float>(0x10000);
  }

  inline void tempo4ppqnTick(uint32_t counter) {
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
      kTap1,
      kTap2,
      kTap3,
      kTap4,
      kNumTaps
  };

  fast_inline void UpdateParameters()
  {
    const float samplerate = runtime_desc_.samplerate;

    mMixSmoother.SetTarget(params_[kParamWet] * 0.01);
    mInputSpreadSmoother.SetTarget(params_[kParamInputMix] * 0.01);
    mOutputSpreadSmoother.SetTarget(params_[kParamSpread] * 0.01);

    const float timeScale = (params_[kParamTapTimeScale] * 0.01f);
    const float primaryTapScale = 0.5f + clipmaxf(timeScale, 0.5f);
    const float secondaryTapScale = timeScale * 0.74f + 0.25f;
    const float delayTime = params_[kParamDelayTime];
    mDelayTime[kTap1] = clipminf(1.f, delayTime * samplerate * 0.001f);
    mDelayTime[kTap2] = clipminf(1.f, delayTime * samplerate * 0.001f * primaryTapScale);
    mDelayTime[kTap3] = clipminf(1.f, delayTime * samplerate * 0.001f * secondaryTapScale);
    mDelayTime[kTap4] = clipminf(1.f, delayTime * samplerate * 0.001f * secondaryTapScale);

    CookFilterCoeffs();

    mPrimaryFeedbackSmoother.SetTarget(CalculatePrimaryFeedback(CalculateFeedback((delayTime * primaryTapScale - 1) * mDelayTimeRange)));
    mSecondaryFeedbackSmoother.SetTarget(CalculateSecondaryFeedback(CalculateFeedback((delayTime * secondaryTapScale - 1) * mDelayTimeRange)));
  }

  fast_inline float CalculatePrimaryFeedback(const float feedback)
  {
    return (feedback < 0.3f) ? feedback * 1.666 : ((feedback < 0.75f) ? 0.556 * feedback + 0.333 : feedback);
  }

  fast_inline float CalculateSecondaryFeedback(const float feedback)
  {
    return (feedback < 0.3f) ? 0.f : ((feedback < 0.75f) ? 1.666 * feedback - 0.5f : feedback);
  }

  fast_inline void CookFilterCoeffs()
  {
    const float inverseSamplerate = 1.f / runtime_desc_.samplerate;

    // exponential smoothing
    mFilterMixSmoother.SetTarget(params_[kParamTone] < 0);
    float cutoff = params_[kParamTone] * 0.01;
    mCutoffZ += (cutoff - mCutoffZ) * 0.2;

    float lpfCutoff = clipmaxf(mCutoffZ + 1.f, 1.f);
    lpfCutoff = (lpfCutoff * lpfCutoff) * 18500.f + 500.f;

    float hpfCutoff = clipminf(mCutoffZ, 0.f);
    hpfCutoff = (hpfCutoff * hpfCutoff) * 9980.f + 20.f;

    const float lpfK = dsp::BiQuad::Coeffs::tanPiWc(dsp::BiQuad::Coeffs::wc(lpfCutoff, inverseSamplerate));
    const float hpfK = dsp::BiQuad::Coeffs::tanPiWc(dsp::BiQuad::Coeffs::wc(hpfCutoff, inverseSamplerate));
    
    float reso = params_[kParamResonance] * 0.01;
    const float q = (reso * reso) * 4.5f + 0.5f;

    dsp::BiQuad::Coeffs dummy;
    dummy.setSOLP(lpfK, q);
    mOutputFilterCoeffs.set_coeffs(dummy, kOutputLpf1);
    mOutputFilterCoeffs.set_coeffs(dummy, kOutputLpf2);

    dummy.setSOHP(hpfK, q);
    mOutputFilterCoeffs.set_coeffs(dummy, kOutputHpf1);
    mOutputFilterCoeffs.set_coeffs(dummy, kOutputHpf2);
  }

  fast_inline float CalculateFeedback(float delayTimeNormalized)
  {
    // reduce feedback at very short delay times
    const float delayNormalizedScale = clipminmaxf(0.939f, 6.1f * delayTimeNormalized + 0.878f, 1.f);
    float fbParam = params_[kParamFeedback] * 0.01;
    return (fbParam * delayNormalizedScale);
  }

  std::atomic_uint_fast32_t flags_;

  unit_runtime_desc_t runtime_desc_;

  float mTempo;

  int32_t params_[kNumParams];

  dsp::LinearSmoother mMixSmoother;
  dsp::LinearSmoother mInputSpreadSmoother;
  dsp::LinearSmoother mOutputSpreadSmoother;
  dsp::LinearSmoother mPrimaryFeedbackSmoother;
  dsp::LinearSmoother mSecondaryFeedbackSmoother;
  dsp::LinearSmoother mFilterMixSmoother;

  float mCutoffZ;
  float mDelayTimeRange;
  float mDelayTime[kNumTaps];
  float mDelayTimeZ[kNumTaps];
  dsp::DelayLine mDelayLine;
  
  enum
  {
    kOutputLpf1,
    kOutputLpf2,
    kOutputHpf1,
    kOutputHpf2,
    kNumOutputFilters
  };

  dsp::ParallelBiQuad<kNumOutputFilters> mOutputFilters;
  dsp::ParallelBiQuad<kNumOutputFilters>::ParallelCoeffs mOutputFilterCoeffs;

  dsp::DelayLine mDelayLine1;
  dsp::DelayLine mDelayLine2;

  float * allocated_buffer_;
  
  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
  const float mDelayTimeSmoothingCoeff;
};
