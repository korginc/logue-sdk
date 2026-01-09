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
 *  File: breveR.h
 *
 *  A reverb based on Natural Sounding Artificial Reverberation by Shroeder https://secure.aes.org/forum/pubs/journal/?ID=1084
 *  with some minor modifications (pre delay, extra all pass filters to increase echo density, direct output from comb filters 
 *  to increase stereo spacialization) to address shortcomings of the algorithm outside of the settings defined in his paper.
 *  The pre delay buffer can also be used as a reverse buffer to make the effect a little more interesting. 
 * 
 *  This is mostly intended as example/educational code to give developers who are new to reverb and fixed point math a 
 *  starting place for investigating those two topics.
 * 
 *  Davis Sprague / Korg / 2025
 * 
 */

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <iostream>

#include "utils/buffer_ops.h" // for buf_clr_f32()
#include "utils/int_math.h"   // for clipminmaxi32()
#include "utils/fixed_math.h"
#include "utils/mk2_utils.h"
#include "runtime.h"
#include "unit_revfx.h"
#include "macros.h"
#include "dsp/LinearSmoother.h"
#include "dsp/simplelfo.hpp"

class breveR {
 public:
  /*===========================================================================*/
  /* Public Data Structures/Types/Enums. */
  /*===========================================================================*/

  enum {
    BUFFER_LENGTH = REVFX_MEMORY_SIZE,
  };

  enum {
    kParamTime = 0U,
    kParamDepth,
    kParamHiDamp, 
    kParamPreDelay,
    kParamSize,
    kParamDiffusion,
    kParamReverse,
    kParamMix,
    kNumParams
  };
  
  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  breveR(void):
  mTempo(120),
  mReverse(0),
  mWriteIndex(0),
  mReadIndex1(0),
  mReadIndex2(0),
  mPreDelayTime(0),
  mDiffusionMix(0),
  allocated_buffer_(nullptr),
  mPreDelayLine(nullptr),
  mCombLine(nullptr),
  mApfLine(nullptr),
  mPreDelaySize(1 << 15),
  mPreDelayMask(mPreDelaySize - 1),
  mCombSize(1 << 15), // (4096 x 4) x2 for reverse
  mCombMask((mCombSize >> 2) - 1),
  mApfSize(1 << 14), // 2048 samples x4
  mApfMask((mApfSize >> 2) - 1)
  {

  }

  ~breveR(void) 
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
    mReverseLfo1.reset();
    mReverseLfo2.reset();
    mReverseLfo2.phi0 = -0x40000000; // offset lfo 2 by 90 degrees

    int16_t * delayLine = reinterpret_cast<int16_t *>(allocated_buffer_);
    mPreDelayLine = delayLine;
    delayLine += mPreDelaySize;

    mCombLine = delayLine;
    delayLine += mCombSize;

    mApfLine = delayLine;
    delayLine += mApfSize;

    buf_clr_u32(mEarlyReflectionsTimes, 4);
    buf_clr_u32(mCombTimes, 4);
    buf_clr_u32(mApfTimes, 4);

    for(int i = 0; i < 4; i++)
    {
      mCombLpfZ[i] = 0; 
      mApfZ[i] = 0;
      mApfGains[i] = 0;    
      mCombGains[i] = 0;    
      mCombLpfCoeffs[i] = 0;
    }
  }

  inline void Resume() {
    // Note: Effect will resume and exit suspend state. Usually means the synth
    // was selected and the render callback will be called again
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

    for (; out_p != out_e; in_p += 2, out_p += 2) 
    {
      // Process samples here
      const float dryL = in_p[0];
      const float dryR = in_p[1];

      float sig = (dryL + dryR) * mTrimSmoother.Process();
      int16_t fixedSig = f32_to_q15(sig) >> 1; // headroom
                              
      // get reverse if switched on
      int32_t prevLfo1 = mReverseLfo1.phi0;
      int32_t prevLfo2 = mReverseLfo2.phi0;
      mReverseLfo1.cycle();
      mReverseLfo2.cycle();
      const bool lfo1Reset = (prevLfo1 > mReverseLfo1.phi0);
      const bool lfo2Reset = (prevLfo2 > mReverseLfo2.phi0);
      int16_t reverseWindow1 = mReverse ? f32_to_q15(si_fabsf(mReverseLfo1.sine_bi())) : 0x7FFF;
      int16_t reverseWindow2 = mReverse ? f32_to_q15(si_fabsf(mReverseLfo2.sine_bi())) : 0x0000;
      
      mPreDelayLine[mWriteIndex & mPreDelayMask] = fixedSig;
      int16_t preDelayOut = mReverse ? mPreDelayLine[mReadIndex1 & mPreDelayMask] : mPreDelayLine[(mWriteIndex + mPreDelayTime) & mPreDelayMask];
      preDelayOut = q15mul(preDelayOut, reverseWindow1) + q15mul(mPreDelayLine[mReadIndex2 & mPreDelayMask], reverseWindow2);
      
      // parallel comb filters
      int16_t comb1 = mCombLine[((mWriteIndex + mCombTimes[0]) & mCombMask) * 4 + 0];
      int16_t comb2 = mCombLine[((mWriteIndex + mCombTimes[1]) & mCombMask) * 4 + 1];
      int16_t comb3 = mCombLine[((mWriteIndex + mCombTimes[2]) & mCombMask) * 4 + 2];
      int16_t comb4 = mCombLine[((mWriteIndex + mCombTimes[3]) & mCombMask) * 4 + 3];

      comb1 = q15add(preDelayOut, q15mul(comb1, mCombGains[0]));
      comb2 = q15add(preDelayOut, q15mul(comb2, mCombGains[1]));
      comb3 = q15add(preDelayOut, q15mul(comb3, mCombGains[2]));
      comb4 = q15add(preDelayOut, q15mul(comb4, mCombGains[3]));

      // high damp in comb feedback
      mCombLpfZ[0] = q15sub(q15mul(q15add(mCombLpfZ[0], comb1), mCombLpfCoeffs[0]), comb1);
      mCombLpfZ[1] = q15sub(q15mul(q15add(mCombLpfZ[1], comb2), mCombLpfCoeffs[1]), comb2);
      mCombLpfZ[2] = q15sub(q15mul(q15add(mCombLpfZ[2], comb3), mCombLpfCoeffs[2]), comb3);
      mCombLpfZ[3] = q15sub(q15mul(q15add(mCombLpfZ[3], comb4), mCombLpfCoeffs[3]), comb4);

      // write comb input
      mCombLine[(mWriteIndex & mCombMask) * 4 + 0] = mCombLpfZ[0];
      mCombLine[(mWriteIndex & mCombMask) * 4 + 1] = mCombLpfZ[1];
      mCombLine[(mWriteIndex & mCombMask) * 4 + 2] = mCombLpfZ[2];
      mCombLine[(mWriteIndex & mCombMask) * 4 + 3] = mCombLpfZ[3];
        
      // output allpass filters
      int16_t combOutGain = 0x1FFF;
      int16_t apfIn = q15add(q15mul(comb1, combOutGain), q15mul(comb2, combOutGain));
      apfIn = q15add(apfIn, q15mul(comb2, combOutGain));
      apfIn = q15add(apfIn, q15mul(comb3, combOutGain));
              
      // use extra delay between filters to help with parallelization
      int16_t apf1Out = mApfLine[((mWriteIndex + mApfTimes[0]) & mApfMask) * 4 + 0];
      int16_t apf2Out = mApfLine[((mWriteIndex + mApfTimes[1]) & mApfMask) * 4 + 1];
      int16_t apf3Out = mApfLine[((mWriteIndex + mApfTimes[2]) & mApfMask) * 4 + 2];
      int16_t apf4Out = mApfLine[((mWriteIndex + mApfTimes[3]) & mApfMask) * 4 + 3];
      int16_t apf1In = q15add(apfIn   , q15mul(apf1Out, mApfGains[0]));
      int16_t apf2In = q15add(mApfZ[0], q15mul(apf2Out, mApfGains[1]));
      int16_t apf3In = q15add(mApfZ[1], q15mul(apf3Out, mApfGains[2]));
      int16_t apf4In = q15add(mApfZ[2], q15mul(apf4Out, mApfGains[3]));
      mApfLine[(mWriteIndex & mApfMask) * 4 + 0] = apf1In;
      mApfLine[(mWriteIndex & mApfMask) * 4 + 1] = apf2In;
      mApfLine[(mWriteIndex & mApfMask) * 4 + 2] = apf3In;
      mApfLine[(mWriteIndex & mApfMask) * 4 + 3] = apf4In;
      mApfZ[0] = q15sub(apf1Out, q15mul(apf1In, mApfGains[0]));
      mApfZ[1] = q15sub(apf2Out, q15mul(apf2In, mApfGains[1]));
      mApfZ[2] = q15sub(apf3Out, q15mul(apf3In, mApfGains[2]));
      mApfZ[3] = q15sub(apf4Out, q15mul(apf4In, mApfGains[3]));

      int16_t monoOut = q15mul(apfIn, mApfOutputGains[0]);
      monoOut = q15add(monoOut, q15mul(mApfZ[0], mApfOutputGains[1]));
      monoOut = q15add(monoOut, q15mul(mApfZ[1], mApfOutputGains[2]));

      int16_t fixedOutL = q15add(monoOut, q15mul(mApfZ[2], mApfOutputGains[3]));
      int16_t fixedOutR = q15add(monoOut, q15mul(mApfZ[3], mApfOutputGains[3]));
      fixedOutL = q15add(fixedOutL, (mCombLine[((mWriteIndex + mStereoOutTimes[0]) & mCombMask) * 4 + 0]) >> 1);
      fixedOutL = q15add(fixedOutL, (mCombLine[((mWriteIndex + mStereoOutTimes[1]) & mCombMask) * 4 + 1]) >> 1);
      fixedOutR = q15add(fixedOutR, (mCombLine[((mWriteIndex + mStereoOutTimes[2]) & mCombMask) * 4 + 2]) >> 1);
      fixedOutR = q15add(fixedOutR, (mCombLine[((mWriteIndex + mStereoOutTimes[3]) & mCombMask) * 4 + 3]) >> 1);
      float outL = q15_to_f32(fixedOutL);
      float outR = q15_to_f32(fixedOutR);

      const float wet = mMixSmoother.Process();
      const float dry = 1.f - si_fabsf(wet);      
      out_p[0] = dryL * dry + outL * wet;
      out_p[1] = dryR * dry + outR * wet; 

      mReadIndex1++;
      mReadIndex2++;
      mReadIndex1 = lfo1Reset ? mWriteIndex : mReadIndex1;
      mReadIndex2 = lfo2Reset ? mWriteIndex : mReadIndex2;
      mWriteIndex--;
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

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getParameterStrValue
    
    static const char * fwdRev[2] = {
      "Fwd",
      "Rev",
    };
    
    switch (index) {
      case kParamReverse:
      {
        return fwdRev[value];
        break;
      }

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
  void UpdateParameters()
  {
    mMixSmoother.SetTarget(params_[kParamMix] * 0.01);
    mTrimSmoother.SetTarget(params_[kParamDepth] * 0.01);
    
    const float size = params_[kParamSize] * 0.01;
    
    mReverse = params_[kParamReverse];

    // shroeder suggests incommesurate delay times of a range of 1x ~ 1.5x.
    // In this implementation, the "comb size" parameter is defining the longest delay time,
    // so the minimum delay time will be (1.f / 1.5) * comb time, tweaked as necessary to
    // insure the delay times are not divisible by one another
    const float combSize = 0.1 + 0.9 * size;
    const float diff = (1.f - (1.f / 1.5)) / 3.f;
    float timeScale = 1.f;
    uint32_t maxTime = (((mCombMask + 1) >> 1) - 1);
    mCombTimes[0] = static_cast<uint32_t>(maxTime * combSize - 1);
    timeScale -= diff;
    mCombTimes[1] = static_cast<uint32_t>(mCombTimes[0] * timeScale);
    timeScale -= diff;
    mCombTimes[2] = static_cast<uint32_t>(mCombTimes[0] * timeScale);
    timeScale -= diff;
    mCombTimes[3] = static_cast<uint32_t>(mCombTimes[0] * timeScale);
    
    // tap the comb filter delay lines to create a wider stereo field
    // left
    mStereoOutTimes[0] = static_cast<uint32_t>(maxTime * (0.5 + 0.4 * size));
    mStereoOutTimes[1] = static_cast<uint32_t>(maxTime * (0.15 + 0.1 * size));
    // right
    mStereoOutTimes[2] = static_cast<uint32_t>(maxTime * (0.475 + 0.325 * size));
    mStereoOutTimes[3] = static_cast<uint32_t>(maxTime * (0.2 + 0.1 * size));

    // Time (parameter) and delay time are in seconds
    // Time = 3 * delayTime / -log(gain)
    // log(gain) = -3 * delayTime / Time
    // gain = 10^(-3 * delayTime / Time)
    const float samplerate = runtime_desc_.samplerate;
    const float inverseSamplerate = 1.f / runtime_desc_.samplerate;
    const float maxReverbTimeSeconds = 10.f;
    const float minReverbTimeSeconds = 0.1f;
    const float time = 1.f / ((params_[kParamTime] * 0.01) * (maxReverbTimeSeconds - minReverbTimeSeconds) + minReverbTimeSeconds);
    const float comb1Time = (mCombTimes[0] * inverseSamplerate);
    const float comb2Time = (mCombTimes[1] * inverseSamplerate);
    const float comb3Time = (mCombTimes[2] * inverseSamplerate);
    const float comb4Time = (mCombTimes[3] * inverseSamplerate);

    // If fastpowf is too computationally expensive, pre-calculating these gain values and 
    // using a table lookup is an easy way to optimize coefficient calculation
    float comb1Gain = fastpowf(10.f, -3.f * comb1Time * time);
    float comb2Gain = fastpowf(10.f, -3.f * comb2Time * time);
    float comb3Gain = fastpowf(10.f, -3.f * comb3Time * time);
    float comb4Gain = fastpowf(10.f, -3.f * comb4Time * time);
    mCombGains[0] = f32_to_q15(comb1Gain);
    mCombGains[1] = f32_to_q15(comb2Gain);
    mCombGains[2] = f32_to_q15(comb3Gain);
    mCombGains[3] = f32_to_q15(comb4Gain);

    // Potential improvement: apply a spread/diffusion parameter here so each comb has a slightly different frequency response
    float highDamp = 1.f - (params_[kParamHiDamp] * 0.01);
    highDamp *= highDamp;
    highDamp = highDamp * (20000.f - 500.f) + 500.f;

    // Same as fastpowf, table lookup may be more efficient for fastexpf (and all other transcendental functions)
    float coeff = fastexpf((double)(-2.f * (float)M_PI * highDamp * inverseSamplerate));
    mCombLpfCoeffs[0] = f32_to_q15(coeff);
    mCombLpfCoeffs[1] = f32_to_q15(coeff);
    mCombLpfCoeffs[2] = f32_to_q15(coeff);
    mCombLpfCoeffs[3] = f32_to_q15(coeff);
    
    // schroeder suggests delay times of 5 and 1.7 ms for all pass filters, but at long delay times the echo density
    // is not sufficient with only two all pass filters + four combs. Increasing the number of all pass filters
    // helps us increase the echo density and provide some stereo field enhancement
    float apfTime1 = 15.3f;
    float apfTime2 = 5.f;
    float apfTime3 = 1.8f;
    float apfTime4 = 1.6f;
    mApfTimes[0] = (apfTime1 * samplerate * 0.001);
    mApfTimes[1] = (apfTime2 * samplerate * 0.001);
    mApfTimes[2] = (apfTime3 * samplerate * 0.001);
    mApfTimes[3] = (apfTime4 * samplerate * 0.001);

    const float diffusion = params_[kParamDiffusion] * 0.01f;
    mDiffusionMix = f32_to_q15(clipmaxf(diffusion * 2.f, 1.f));
    
    float diffusionCoeffSpread = clipminf((diffusion - 0.5f) * 2.f, 0.f);
    float baseCoeff = 0.6f;
    mApfGains[0] = f32_to_q15(baseCoeff + 0.2 * diffusionCoeffSpread);
    mApfGains[1] = f32_to_q15(baseCoeff - 0.3 * diffusionCoeffSpread);
    mApfGains[2] = f32_to_q15(baseCoeff + 0.05 * diffusionCoeffSpread);
    mApfGains[3] = f32_to_q15(baseCoeff - 0.1 * diffusionCoeffSpread);
    
    const float numStages = 4.f - 1.f;
    const float invNumStages = 1.f / numStages;
    mApfOutputGains[0] = f32_to_q15(clipminmaxf(0.f, 1.f - numStages * si_fabsf(diffusion - (0.f * invNumStages)), 1.f));
    mApfOutputGains[1] = f32_to_q15(clipminmaxf(0.f, 1.f - numStages * si_fabsf(diffusion - (1.f * invNumStages)), 1.f));
    mApfOutputGains[2] = f32_to_q15(clipminmaxf(0.f, 1.f - numStages * si_fabsf(diffusion - (2.f * invNumStages)), 1.f));
    mApfOutputGains[2] = f32_to_q15(clipminmaxf(0.f, 1.f - numStages * si_fabsf(diffusion - (3.f * invNumStages)), 1.f));

    mPreDelayTime = (params_[kParamPreDelay] * 0.1) * samplerate * 0.001;
    
    float reverseFreq = clipminmaxf(2400.f, mPreDelayTime, mPreDelaySize * 0.5);
    reverseFreq = samplerate / reverseFreq;
    mReverseLfo1.setF0(reverseFreq, inverseSamplerate); 
    mReverseLfo2.setF0(reverseFreq, inverseSamplerate);
  }

  std::atomic_uint_fast32_t flags_;

  unit_runtime_desc_t runtime_desc_;

  dsp::LinearSmoother mMixSmoother;
  dsp::LinearSmoother mTrimSmoother;
  dsp::SimpleLFO mReverseLfo1;
  dsp::SimpleLFO mReverseLfo2;

  float mTempo;

  int32_t params_[kNumParams];

  bool     mReverse;
  uint32_t mWriteIndex;
  uint32_t mReadIndex1;
  uint32_t mReadIndex2;
  uint32_t mPreDelayTime;
  uint32_t mEarlyReflectionsTimes[4];
  uint32_t mCombTimes[4];
  uint32_t mApfTimes[4];
  uint32_t mStereoOutTimes[4];

  int16_t mDiffusionMix;
  int16_t mCombLpfZ[4];
  int16_t mApfZ[4];
  int16_t mApfGains[4];
  int16_t mCombGains[4];
  int16_t mCombLpfCoeffs[4];
  int16_t mApfOutputGains[4];

  float * allocated_buffer_;
  int16_t * mPreDelayLine;
  int16_t * mCombLine;
  int16_t * mApfLine;
  
  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
  const uint32_t mPreDelaySize;
  const uint32_t mPreDelayMask;
  const uint32_t mCombSize;
  const uint32_t mCombMask;
  const uint32_t mApfSize;
  const uint32_t mApfMask;
};
