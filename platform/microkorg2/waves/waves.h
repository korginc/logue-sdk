#pragma once

#include <atomic>

#include "unit_osc.h"
#include "runtime.h"
#include "utils/mk2_utils.h"
#include "dsp/mk2_biquad.hpp"
#include "utils/io_ops.h"
#include "waves_common.h"
#include "macros.h"
#include <iostream>
#include "SystemPaths.h"

#ifndef fast_inline
#define fast_inline __attribute__((optimize("Ofast"), always_inline)) inline
#endif

class Waves
{
public: 
  /*===========================================================================*/
  /* Public Data Structures/Types. */
  /*===========================================================================*/

  struct Params {
    enum {
      k_shape = 0U,
      k_sub_mix,
      k_wave_a,
      k_wave_b,
      k_sub_wave,
      k_ring_mix,
      k_bit_crush,
      k_drift,
    };
    
    float    sub_mix{0.05f};
    float    ring_mix{0.f};
    float    bit_crush{0.f};
    float    shape{0.f};
    float    shape_lfo_rate{0.f};
    float    shape_lfo_depth{0.f};
    float    drift{1.25f};
    uint8_t  wave_a{0};
    uint8_t  wave_b{0};
    uint8_t  sub_wave{0};
    uint8_t  padding{0};
    
    void reset() {
      sub_mix = 0.05f;
      ring_mix = 0.f;
      bit_crush = 0.f;
      shape = 0.f;
      drift = 1.25f;
      wave_a = 0;
      wave_b = 0;
      sub_wave = 0;
      padding = 0;
    }
  };
  
  struct State {

    enum {
      k_flags_none    = 0,
      k_flag_wave_a   = 1<<1,
      k_flag_wave_b   = 1<<2,
      k_flag_sub_wave  = 1<<3,
      k_flag_ring_mix  = 1<<4,
      k_flag_bit_crush = 1<<5,
      k_flag_reset    = 1<<6
    };
    
    const float              *wave_a;        // selected wave a data
    const float              *wave_b;        // selected wave b data
    const float              *sub_wave;      // selected sub wave data
    float                     phi_a[kMk2MaxVoices];   // wave a phase
    float                     phi_b[kMk2MaxVoices];   // wave b phase
    float                     phi_sub[kMk2MaxVoices]; // sub wave phase
    float                     w0_a[kMk2MaxVoices];    // wave a phase increment
    float                     w0_b[kMk2MaxVoices];    // wave b phase increment
    float                     w0_sub[kMk2MaxVoices];  // sub wave phase increment
    float                     shapeMod[kMk2MaxVoices];     // target lfo value
    float                     shapeModZ[kMk2MaxVoices];    // current interpolated lfo value
    float                     dither;        // dithering amount before bit reduction
    float                     bit_res;       // bit depth scaling factor
    float                     bit_res_recip; // bit depth scaling reciprocal, returns signal to 0.-1.f after scaling/rounding
    float                     imperfection;  // tuning imperfection
    std::atomic_uint_fast32_t flags;         // flags passed to audio processing thread
    
    State(void) :
      wave_a(wavesA[0]),
      wave_b(wavesD[0]),
      sub_wave(wavesA[0]),
      dither(0.f),
      bit_res(1.f),
      bit_res_recip(1.f),
      flags{k_flags_none}
    {
      buf_fill_f32(w0_a, 440.f * k_samplerate_recipf, kMk2MaxVoices);
      buf_fill_f32(w0_b, 440.f * k_samplerate_recipf, kMk2MaxVoices);
      buf_fill_f32(w0_sub, 220.f * k_samplerate_recipf, kMk2MaxVoices);
      buf_clr_f32(shapeMod, kMk2MaxVoices);
      buf_clr_f32(shapeModZ, kMk2MaxVoices);

      Reset();
      imperfection = osc_white() * 1.0417e-006f; // +/- 0.05Hz@48KHz
    }
    
    inline void Reset(void) {
      buf_clr_f32(phi_a, kMk2MaxVoices);
      buf_clr_f32(phi_b, kMk2MaxVoices);
      buf_clr_f32(phi_sub, kMk2MaxVoices);
      buf_cpy_f32(shapeModZ, shapeMod, kMk2MaxVoices);
    }
  };

  enum
  {
    kModDestShape,
    // kModDestSubMix,
    // kModDestSubWave,
    // kModDestRingMix,
    // kModDestBitCrush,
    kNumModDest
  };
  
  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Waves(void)
  {}
  
  ~Waves(void) 
  {}

  inline int8_t Init(const unit_runtime_desc_t * desc) 
  {
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
    // if (desc->output_channels != 2)  // should be stereo output
    //   return k_unit_err_geometry;

    // Cache runtime descriptor to keep access to API hooks
    runtime_desc_ = *desc;

    // Initialize pre/post filter coefficients
    prelpf_.mCoeffs.setPoleLP(0.9f);
    postlpf_.mCoeffs.setFOLP(osc_tanpif(0.45f));

    // Make sure parameters are reset to default values
    params_.reset();

    return k_unit_err_none;
  }

  inline void Teardown() 
  {
    // Note: cleanup and release resources if any
  }

  inline void Reset() 
  {
    state_.Reset();
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

  fast_inline void Process(float * out, size_t frames) 
  {
    State &s = state_;
    const Waves::Params &p = params_;
    const unit_runtime_osc_context_t *ctxt = static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);
    
    // Handle events.
    {      
      const uint32_t flags = s.flags.exchange(State::k_flags_none, std::memory_order_relaxed);
      updateWaves(flags);
      
      if (flags & State::k_flag_reset)
        s.Reset();
            
      if (flags & State::k_flag_bit_crush) {
        s.dither = p.bit_crush * 2e-008f;
        s.bit_res = osc_bitresf(p.bit_crush);
        s.bit_res_recip = 1.f / s.bit_res;
      }
    }
   
    for(int i = 0; i < ctxt->voiceLimit; i++)
    {
      uint8_t noteWhole = static_cast<uint8_t>(ctxt->pitch[i]);
      float frac = ctxt->pitch[i] - static_cast<float>(noteWhole);
      updatePitch(osc_w0f_for_note(noteWhole, static_cast<uint8_t>(frac * 0xFF)), i);
      ProcessX1(out, i, frames);
    }
  }

  inline void setParameter(uint8_t index, int32_t value) 
  {
    Params &p = params_;
    State &s = state_;
    
    switch (index) {
    case Params::k_shape:
      p.shape = 0.005f + param_10bit_to_f32(value) * 0.99f;
      break;

    case Params::k_sub_mix:
      p.sub_mix = 0.05f + param_10bit_to_f32(value) * 0.90f;
      break;
            
    case Params::k_wave_a: {
      p.wave_a = value % WAVE_A_CNT;
      s.flags.fetch_or(State::k_flag_wave_a);
    }
    break;
    
    case Params::k_wave_b: {
      p.wave_b = value % WAVE_B_CNT;
      s.flags.fetch_or(State::k_flag_wave_b);
    }
    break;
    
    case Params::k_sub_wave:
      p.sub_wave = value % SUB_WAVE_CNT;
      s.flags.fetch_or(State::k_flag_sub_wave);
      break;
        
    case Params::k_ring_mix:
      p.ring_mix = clip01f(value * 0.001f);
      break;
    
    case Params::k_bit_crush:
      p.bit_crush = clip01f(value * 0.001f);
      s.flags.fetch_or(State::k_flag_bit_crush);
      break;

    case Params::k_drift:
      p.drift = 1.f + value * 0.001f;
      break;
      
    default:
      break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const 
  {
    const Params &p = params_;
    
    switch (index) {
    case Params::k_shape:
      //  min, max,  center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   1023, 0,      0,       k_unit_param_type_none, 0,    0,          0,          {"SHAPE"}},
      return param_f32_to_10bit((p.shape - 0.005) / 0.99f);

    case Params::k_sub_mix:
      //  min, max,  center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   1023, 0,      0,       k_unit_param_type_none, 0,    0,          0,          {"SUB"}},
      return param_f32_to_10bit((p.sub_mix - 0.05f) / 0.90f);
            
    case Params::k_wave_a: 
      //  min, max,          center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   WAVE_A_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"WAVE A"}},
      return p.wave_a;
    
    case Params::k_wave_b:
      //  min, max,          center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   WAVE_B_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"WAVE B"}},
      return p.wave_b;
    
    case Params::k_sub_wave:
      //  min, max,            center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   SUB_WAVE_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"SUB WAVE"}},
      return p.sub_wave;
          
    case Params::k_ring_mix:
      //  min, max,  center, default, type,                      frac, frac. mode, <reserved>, name
      // {0,   1000, 0,      0,       k_unit_param_type_percent, 1,    1,          0,          {"RING MIX"}},
      return si_roundf(p.ring_mix * 1000);
    
    case Params::k_bit_crush:
      //  min, max,  center, default, type,                      frac, frac. mode, <reserved>, name
      // {0,   1000, 0,      0,       k_unit_param_type_percent, 1,    1,          0,          {"BIT CRUSH"}},
      return si_roundf(p.bit_crush * 1000);

    case Params::k_drift:
      //  min, max,  center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   1000, 0,      250,     k_unit_param_type_none, 1,    1,          0,          {"DRIFT"}},
      return si_roundf((p.drift - 1.f) * 1000);
      
    default:
      break;
    }

    return -0x7FFFFFFF; // Note: will be handled as invalid
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const 
  {
    (void)index;
    (void)value;
    return nullptr;
  }
  inline const uint8_t * getParameterBmpValue(uint8_t index,
                                              int32_t value) const 
  {
    (void)value;
    switch (index) 
    {
      // Note: Bitmap memory must be accessible even after function returned.
      //       It can be assumed that caller will have copied or used the bitmap
      //       before the next call to getParameterBmpValue
      // Note: Not yet implemented upstream
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

  void unit_platform_exclusive(uint8_t messageId, void * data, uint32_t dataSize) 
  {
    const unit_runtime_osc_context_t * ctxt = static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);
    switch (messageId)
    {
      case kMk2PlatformExclusiveModData:
      {
        float * voiceFreqData = GetModData(data);
        const int modDestIndexMax = dataSize;
        buf_cpy_f32(&voiceFreqData[clipmaxi32(ctxt->voiceLimit * kModDestShape, modDestIndexMax)], state_.shapeMod, ctxt->voiceLimit);
        break;
      }
    
      case kMk2PlatformExclusiveModDestName:
      {
        char * modName = GetModDestNameData(data);
        uint8_t modIndex = modName[0];
        switch (modIndex)
        {
          case kModDestShape:
          {
            modName[1] = 'S';
            modName[2] = 'h';
            modName[3] = 'a';
            modName[4] = 'p';
            modName[5] = 'e';
            break;
          }

          // case kModDestSubMix:
          // {
          //   break;
          // }

          // case kModDestSubWave:
          // {
          //   break;
          // }

          // case kModDestRingMix:
          // {
          //   break;
          // }

          // case kModDestBitCrush:
          // {
          //   break;
          // }
        }
        break;
      }
      default:
        break;  
    }
  }

 private:
  /*===========================================================================*/
  /* Private Classes. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Private Member Variables. */
  /*===========================================================================*/

  State       state_;
  Params      params_;
  dsp::ParallelBiQuad<kMk2MaxVoices> prelpf_, postlpf_;
  unit_runtime_desc_t runtime_desc_;

  std::atomic_uint_fast32_t flags_;

  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  fast_inline void updatePitch(float w0, int voice) {
    w0 += state_.imperfection;
    const float drift = params_.drift;
    state_.w0_a[voice] = w0;
    // Alt. osc with slight phase drift (0.25Hz@48KHz)
    state_.w0_b[voice] = w0 + drift * 5.20833333333333e-006f;
    // Sub one octave down, with a phase drift (0.15Hz@48KHz)
    state_.w0_sub[voice] = 0.5f * w0 + drift * 3.125e-006f;
  }

  fast_inline void updateWaves(const uint32_t flags) {
    if (flags & State::k_flag_wave_a) {
      static const uint8_t k_a_thr = k_waves_a_cnt;
      static const uint8_t k_b_thr = k_a_thr + k_waves_b_cnt;
      
      uint8_t idx = params_.wave_a;
      const float * const * table;
      
      if (idx < k_a_thr) {
        table = wavesA;
      }
      else if (idx < k_b_thr) {
        table = wavesB;
        idx -= k_a_thr;
      }
      else { 
        table = wavesC;
        idx -= k_b_thr;
      }
      state_.wave_a = table[idx];
    }
    if (flags & State::k_flag_wave_b) {
      static const uint8_t k_d_thr = k_waves_d_cnt;
      static const uint8_t k_e_thr = k_d_thr + k_waves_e_cnt;
      
      uint8_t idx = params_.wave_b;
      const float * const * table;
      
      if (idx < k_d_thr) {
        table = wavesD;
      }
      else if (idx < k_e_thr) {
        table = wavesE;
        idx -= k_d_thr;
      }
      else { 
        table = wavesF;
        idx -= k_e_thr;
      }
      
      state_.wave_b = table[idx];
    }
    if (flags & State::k_flag_sub_wave) {
      state_.sub_wave = wavesA[params_.sub_wave];
    }
  }

  void ProcessX1(float * out, const uint32_t voiceNum, const size_t frames)
  {
    State &s = state_;
    const Waves::Params &p = params_;
    const unit_runtime_osc_context_t *ctxt = static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);

    // Temporaries.
    float phi_a = s.phi_a[voiceNum];
    float phi_b = s.phi_b[voiceNum];
    float phi_sub = s.phi_sub[voiceNum];
    const float w0_a = s.w0_a[voiceNum];
    const float w0_b = s.w0_b[voiceNum];
    const float w0_sub = s.w0_sub[voiceNum];
    
    float shape_mod_z = s.shapeModZ[voiceNum];
    const float shape_mod_inc = (s.shapeMod[voiceNum] - shape_mod_z) / frames;
            
    const float sub_mix = p.sub_mix * 0.5011872336272722f;
    const float ring_mix = p.ring_mix;
      
    const int offset = GetBufferOffset(ctxt, voiceNum, frames);
    const float outputTrim = 0.3f;
    for(uint32_t i = 0; i < frames; i++)
    {
      const float wave_mix = clip01f(p.shape+shape_mod_z);
      
      float sig = (1.f - wave_mix) * osc_wave_scanf(s.wave_a, phi_a);
      sig += wave_mix * osc_wave_scanf(s.wave_b, phi_b);
    
      const float sub_sig = osc_wave_scanf(s.sub_wave, phi_sub);
      sig = (1.f - ring_mix) * sig + ring_mix * 1.4125375446227544f * (sub_sig * sig);
      sig += sub_mix * sub_sig;
      sig *= 1.4125375446227544f;
      sig = clip1m1f(fastertanh2f(sig));
    
      sig = prelpf_.process_fo_x1(sig, voiceNum);
      sig += s.dither * osc_white();
      sig = si_roundf(sig * s.bit_res) * s.bit_res_recip;
      sig = postlpf_.process_fo_x1(sig, voiceNum);
      
      write_oscillator_output_x1(out, sig * outputTrim, offset, ctxt->outputStride, i, voiceNum);
    
      phi_a += w0_a;
      phi_a -= (uint32_t)phi_a;
      phi_b += w0_b;
      phi_b -= (uint32_t)phi_b;
      phi_sub += w0_sub;
      phi_sub -= (uint32_t)phi_sub;
      shape_mod_z += shape_mod_inc;
    }

    // Update state
    s.phi_a[voiceNum] = phi_a;
    s.phi_b[voiceNum] = phi_b;
    s.phi_sub[voiceNum] = phi_sub;
    s.shapeModZ[voiceNum] = shape_mod_z;    
  }

  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
};
