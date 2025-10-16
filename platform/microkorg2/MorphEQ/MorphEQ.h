  #pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <arm_neon.h>
#include <math.h>
#include <string>

#include "utils/float_math.h"
#include "utils/int_math.h"
#include "utils/buffer_ops.h"
#include "dsp/simplelfo.hpp"
#include "dsp/mk2_biquad.hpp"
#include "unit_modfx.h"
#include "dsp/LinearSmoother.h"
#include "macros.h"
#include "utils/mk2_utils.h"

#ifndef fast_inline
#define fast_inline __attribute__((optimize("Ofast"), always_inline)) inline
#endif

class MorphEQ 
{
public: 
  /*===========================================================================*/
  /* Public Data Structures/Types. */
  /*===========================================================================*/

  enum {
    kParamGainScale = 0,
    kParamCutoffScale,
    kParamSpreadScale,
    kParamLowGain,
    kParamMidCutoff,
    kParamMidQ,
    kParamMidGain,
    kParamHighGain,
    kNumParams
  };

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  MorphEQ():
  mMidCutoff(1000),
  mPreviousSpread(-1),
  mCrossfadeTarget(1.f),
  mCrossfadeZ(0.f),
  mActiveLeftChannel(0),
  mActiveRightChannel(0),
  mLowCutoff(250),
  mHighCutoff(5000),
  mMinMidQ(50.f),
  mMaxMidQ(3950.f),
  mMinMidFc(40.f),
  mMaxMidFc(18000.f)
  {
    for(int i = 0; i < kPowTableSize; i++)
    {
      const float exponent = -2.f + (float(i)/(kPowTableSize - 1)) * 4.f;
      mPowTable[i] = std::pow(2.f, exponent);
    }
  }

  ~MorphEQ() 
  {}

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
    // if (!desc->hooks.sdram_alloc)
    //   return k_unit_err_memory;
    // float *m = (float *)desc->hooks.sdram_alloc(BUFFER_LENGTH * sizeof(float));
    // if (!m)
    //   return k_unit_err_memory;

    // microkorg2 handles buffer clearing
    // allocated_buffer_ = m;
    
    // Cache the runtime descriptor for later use
    runtime_desc_ = *desc;

    buf_clr_u32(reinterpret_cast<uint32_t *>(params_), kNumParams);

    for(int i = 0; i < kNumParams; i++)
    {
      params_[i] = unit_header.params[i].init;
    }

    mGainSmootherLow.Flush();
    mGainSmootherMid.Flush();
    mGainSmootherHigh.Flush();
    mSpreadSmoother.Flush();
    mCutoffSmoother.Flush();

    mFilter[kLowEQ].flush();
    mFilter[kMidEQ].flush();
    mFilter[kHighEQ].flush();

    mMidCutoffValueStr = stringBuffer;

    // set coeffs to passthrough
    dsp::ExtBiQuad passthrough;
    passthrough.mCoeffs.ff0 = 1.f;
    passthrough.mCoeffs.ff1 = 0.f;
    passthrough.mCoeffs.ff2 = 0.f;
    passthrough.mCoeffs.fb1 = 0.f;
    passthrough.mCoeffs.fb2 = 0.f;
    passthrough.mD0 = 0.f;
    passthrough.mD1 = 0.f;
    passthrough.mW0 = 0.f;
    passthrough.mW1 = 0.f;
    mCoeffs[kLowEQ].set_coeffs(passthrough, 0);
    mCoeffs[kLowEQ].set_coeffs(passthrough, 1);
    mCoeffs[kLowEQ].set_coeffs(passthrough, 2);
    mCoeffs[kLowEQ].set_coeffs(passthrough, 3);

    mCoeffs[kMidEQ].set_coeffs(passthrough, 0);
    mCoeffs[kMidEQ].set_coeffs(passthrough, 1);
    mCoeffs[kMidEQ].set_coeffs(passthrough, 2);
    mCoeffs[kMidEQ].set_coeffs(passthrough, 3);

    mCoeffs[kHighEQ].set_coeffs(passthrough, 0);
    mCoeffs[kHighEQ].set_coeffs(passthrough, 1);
    mCoeffs[kHighEQ].set_coeffs(passthrough, 2);
    mCoeffs[kHighEQ].set_coeffs(passthrough, 3);

    return k_unit_err_none;
  }

  inline void Teardown() 
  {
  }

  inline void Reset() 
  {
    mGainSmootherLow.Flush();
    mGainSmootherMid.Flush();
    mGainSmootherHigh.Flush();
    mSpreadSmoother.Flush();
    mCutoffSmoother.Flush();

    mFilter[kLowEQ].flush();
    mFilter[kMidEQ].flush();
    mFilter[kHighEQ].flush();

    mCrossfadeZ = mCrossfadeTarget;
  }

  inline void Resume() 
  {
    // Note: Synth will resume and exit suspend state. Usually means the synth
    // was selected and the render callback will be called again
    Reset();
  }

  inline void Suspend() 
  {
    // Note: Synth will enter suspend state. Usually means another synth was
    // selected and thus the render callback will not be called
  }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void Render(const float * in, float * out, size_t frames) 
  {
    const float * __restrict in_p = in;
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1);  // assuming stereo output

    UpdateParameters();

    float side = 0;
    const float crossfadeDelta = (mCrossfadeTarget - mCrossfadeZ) / frames;
    for (; out_p != out_e; in_p += 2, out_p += 2) 
    {    
      // process filter bands
      float32x4_t sig = float32x4(in_p[0], in_p[1], in_p[0], in_p[1]);
      sig = mFilter[kLowEQ].process_fo_x4(sig, mCoeffs[kLowEQ]); 
      sig = mFilter[kMidEQ].process_so_x4(sig, mCoeffs[kMidEQ]);
      sig = mFilter[kHighEQ].process_fo_x4(sig, mCoeffs[kHighEQ]);

      // crossfade between active filter and previous filter
      mCrossfadeZ += crossfadeDelta;
      float32x2_t stereoSig = float32x2_mulscal(float32x4_low(sig), (1.f - mCrossfadeZ));
      stereoSig = float32x2_fmulscaladd(stereoSig, float32x4_high(sig), mCrossfadeZ);

      // mid/side spread 
      const float left = f32x2_lane(stereoSig, 0);
      const float right = f32x2_lane(stereoSig, 1);
      float mid = (left + right) * 0.5;
      side = (right - left) * mSpreadSmoother.Process();
        
      // soft clip here to account for large gain from EQ
      out_p[0] = fx_sat_cubicf(clipminmaxf(-1.f, mid + side, 1.f));
      out_p[1] = fx_sat_cubicf(clipminmaxf(-1.f, mid - side, 1.f));
    }
    mCrossfadeZ = mCrossfadeTarget;
  }

  inline void setParameter(uint8_t index, int32_t value) 
  {
    if (index >= kNumParams) return;
    params_[index] = value;
  }

  inline int32_t getParameterValue(uint8_t index) const 
  {
    if (index >= kNumParams) return 0;
    return params_[index];
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const 
  {
    (void)value;
    switch (index) 
    {
      case kParamMidCutoff:
      {    
        if(mMidCutoff < 10000)
        {
          strcpy(mMidCutoffValueStr, std::to_string(static_cast<int>(mMidCutoff)).c_str());
          AddCustomParameterUnit(mMidCutoffValueStr, "Hz", kMicrokorg2FontSizeMedium);
        }
        else
        {
          int whole = static_cast<int>(mMidCutoff/1000);
          int decimal = static_cast<int>((mMidCutoff - (whole * 1000))/10);
          std::string zero = (decimal / 10 == 0) ? "0" : "";
          strcpy(mMidCutoffValueStr, (std::to_string(whole) + "." + std::to_string(decimal) + zero).c_str());
          AddCustomParameterUnit(mMidCutoffValueStr, "kHz", kMicrokorg2FontSizeMedium);
        }
        return mMidCutoffValueStr;
      }
      default:
        break;
    }

    return nullptr;
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }

  inline uint8_t getPresetIndex() const { return 0; }

  /*===========================================================================*/
  /* Static Members. */
  /*===========================================================================*/

  static inline const char * getPresetName(uint8_t idx) 
  {
    (void)idx;
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getPresetName
    return nullptr;
  }

 private:
  /*===========================================================================*/
  /* Private Classes. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Private Member Variables. */
  /*===========================================================================*/

  enum
  {
      kLowEQ,
      kMidEQ,
      kHighEQ,
      kNumBands,

      kChannelLeftA = 0,
      kChannelRightA,
      kChannelLeftB,
      kChannelRightB,
      kNumChannels
  };

  int32_t params_[kNumParams];
  unit_runtime_desc_t runtime_desc_;

  dsp::LinearSmoother mGainSmootherLow;
  dsp::LinearSmoother mGainSmootherMid;
  dsp::LinearSmoother mGainSmootherHigh;
  dsp::LinearSmoother mSpreadSmoother;
  dsp::LinearSmoother mCutoffSmoother;

  dsp::ParallelExtBiQuad<kNumChannels> mFilter[kNumBands]; 
  dsp::ParallelExtBiQuad<kNumChannels>::ParallelCoeffs mCoeffs[kNumBands];
  std::atomic_uint_fast32_t flags_;

  float mMidCutoff;
  float mPreviousSpread;
  float mCrossfadeTarget;
  float mCrossfadeZ;
  int mActiveLeftChannel;
  int mActiveRightChannel;
  const float mLowCutoff;
  const float mHighCutoff;
  const float mMinMidQ;
  const float mMaxMidQ;
  const float mMinMidFc;
  const float mMaxMidFc;
  char * mMidCutoffValueStr;
  char stringBuffer[16];

  enum
  {
    kNumPowTableValues = 128,
    kPowTableSize = kNumPowTableValues + 1
  };
  float mPowTable[kPowTableSize];

  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  void UpdateParameters()
  {
    const float spread = params_[kParamSpreadScale] * 0.01;
    const bool  spreadDirection = spread > 0;
    const float paramSpread = clip01f(si_fabsf(spread) * 2.f);
    const float cutoffSpread = 1.f - 0.2 * paramSpread;
    const float gainSpread = 1.f - 0.05 * paramSpread;
    mSpreadSmoother.SetTarget(si_fabsf(spread));
    
    bool needsUpdate = (mPreviousSpread != spread);
    mPreviousSpread = spread;

    const float gainScale = params_[kParamGainScale] * 0.01;
    mGainSmootherLow.SetTarget((params_[kParamLowGain] * 0.1) * gainScale);
    mGainSmootherMid.SetTarget((params_[kParamMidGain] * 0.1) * gainScale);
    mGainSmootherHigh.SetTarget((params_[kParamHighGain] * 0.1) * gainScale);

    const float previousLowGain = mGainSmootherLow.GetSmoothedValue();
    const float previousMidGain = mGainSmootherMid.GetSmoothedValue();
    const float previousHighGain = mGainSmootherHigh.GetSmoothedValue();
    const float lowGainDB = mGainSmootherLow.Process();
    const float midGainDB = mGainSmootherMid.Process();
    const float highGainDB = mGainSmootherHigh.Process();

    needsUpdate |= (previousLowGain != lowGainDB || previousMidGain != midGainDB || previousHighGain != highGainDB);

    float midQ = params_[kParamMidQ] * 0.01;
    midQ = (midQ - 1.f) * (midQ - 1.f);
    midQ = midQ * mMaxMidQ + mMinMidQ;

    float midCutoff = param_10bit_to_f32(params_[kParamMidCutoff]);
    midCutoff *= midCutoff;
    midCutoff = (mMaxMidFc - mMinMidFc) * midCutoff + mMinMidFc;

    const float previousCutoffScale = mCutoffSmoother.GetSmoothedValue();
    const float target = CookCutoffScale(params_[kParamCutoffScale]);
    mCutoffSmoother.SetTarget(target);
    const float cutoffScale = mCutoffSmoother.Process(); 
    const float lowCutoff = clipminmaxf(40.f, mLowCutoff * cutoffScale, 18000.f);
    const float highCutoff = clipminmaxf(40.f, mHighCutoff * cutoffScale, 18000.f);
    mMidCutoff = clipminmaxf(40.f, midCutoff * cutoffScale, 18000.f);
    needsUpdate |= (previousCutoffScale != cutoffScale);

    mCrossfadeTarget = (needsUpdate) ? 1.f - mCrossfadeTarget : mCrossfadeTarget;
    int leftTarget = (mCrossfadeTarget == 1.f) ? kChannelLeftB : kChannelLeftA;
    int rightTarget = (mCrossfadeTarget == 1.f) ? kChannelRightB : kChannelRightA;
    mActiveLeftChannel = (needsUpdate) ? leftTarget : mActiveLeftChannel;
    mActiveRightChannel =  (needsUpdate) ? rightTarget : mActiveRightChannel;

    const float inverseSamplerate = 1.f / runtime_desc_.samplerate;
    dsp::ExtBiQuad dummy;
    // low
    {
        const float spreadCutoff = lowCutoff * cutoffSpread;
        const float spreadGain = lowGainDB * gainSpread;

        const float lowShelfWcL = dsp::BiQuad::Coeffs::wc(spreadDirection ? spreadCutoff : lowCutoff, inverseSamplerate);
        const float lowShelfKL = dsp::BiQuad::Coeffs::tanPiWc(lowShelfWcL);
        dummy.setFOLS(lowShelfKL, fasterdbampf(spreadDirection ? spreadGain : lowGainDB));
        mCoeffs[kLowEQ].set_coeffs(dummy, mActiveLeftChannel);

        const float lowShelfWcR = dsp::BiQuad::Coeffs::wc(spreadDirection ? lowCutoff : spreadCutoff, inverseSamplerate);
        const float lowShelfKR = dsp::BiQuad::Coeffs::tanPiWc(lowShelfWcR);
        dummy.setFOLS(lowShelfKR, fasterdbampf(spreadDirection ? lowGainDB : spreadGain));
        mCoeffs[kLowEQ].set_coeffs(dummy, mActiveRightChannel);
    }

    // mid
    {
        const float spreadCutoff = mMidCutoff * cutoffSpread;
        const float spreadGain = midGainDB * gainSpread;

        float midWcL = dsp::BiQuad::Coeffs::wc(spreadDirection ? spreadCutoff : mMidCutoff, inverseSamplerate);
        float tempQ = si_tanpif(midQ * M_PI * inverseSamplerate);
        dummy.setSOAPPN2(fx_cosf(midWcL), tempQ, fasterdbampf(spreadDirection ? spreadGain : midGainDB));
        mCoeffs[kMidEQ].set_coeffs(dummy, mActiveLeftChannel);

        float midWcR = dsp::BiQuad::Coeffs::wc(spreadDirection ? mMidCutoff : spreadCutoff, inverseSamplerate);
        tempQ = si_tanpif(midQ * M_PI * inverseSamplerate);
        dummy.setSOAPPN2(fx_cosf(midWcR), tempQ, fasterdbampf(spreadDirection ? midGainDB : spreadGain));
        mCoeffs[kMidEQ].set_coeffs(dummy, mActiveRightChannel);
    }

    // high
    {
        const float spreadCutoff = highCutoff * cutoffSpread;
        const float spreadGain = highGainDB * gainSpread;

        const float highShelfWcL = dsp::BiQuad::Coeffs::wc(spreadDirection ? spreadCutoff : highCutoff, inverseSamplerate);
        const float highShelfKL = dsp::BiQuad::Coeffs::tanPiWc(highShelfWcL);
        dummy.setFOHS(highShelfKL, fasterdbampf(spreadDirection ? spreadGain : highGainDB));
        mCoeffs[kHighEQ].set_coeffs(dummy, mActiveLeftChannel);

        const float highShelfWcR = dsp::BiQuad::Coeffs::wc(spreadDirection ? highCutoff : spreadCutoff, inverseSamplerate);
        const float highShelfKR = dsp::BiQuad::Coeffs::tanPiWc(highShelfWcR);
        dummy.setFOHS(highShelfKR, fasterdbampf(spreadDirection ? highGainDB : spreadGain));
        mCoeffs[kHighEQ].set_coeffs(dummy, mActiveRightChannel);
    }
  }

  fast_inline float CookCutoffScale(int32_t paramValue)
  {
    const float cutoffScaleParamMultiplier = 1.f / (unit_header.params[kParamCutoffScale].max - unit_header.params[kParamCutoffScale].min);
    const float cutoffScaleNormalized = clipminmaxf(0, (paramValue - unit_header.params[kParamCutoffScale].min) * cutoffScaleParamMultiplier, 1.f);
    const float cutoffScaleLookupIndex = cutoffScaleNormalized * kNumPowTableValues;
    const uint32_t cutoffScaleLookupIndexWhole = static_cast<uint32_t>(cutoffScaleLookupIndex);
    const float cutoffScaleFrac = cutoffScaleLookupIndex - cutoffScaleLookupIndexWhole;
    return linintf(cutoffScaleFrac, mPowTable[cutoffScaleLookupIndexWhole + 0], mPowTable[cutoffScaleLookupIndexWhole + 1]);
  }
  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
};
