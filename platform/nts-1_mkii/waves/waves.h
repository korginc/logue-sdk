#pragma once
/*
    BSD 3-Clause License

    Copyright (c) 2018-2023, KORG INC.
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
 *  File: waves.h
 *
 *  Simple Morphing Wavetable Demo Oscillator
 *
 */

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <climits>

#include "unit_osc.h"

#include "waves_common.h"

#include "dsp/biquad.hpp"

class Waves {
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
    float                     phi_a;         // wave a phase
    float                     phi_b;         // wave b phase
    float                     phi_sub;       // sub wave phase
    float                     w0_a;          // wave a phase increment
    float                     w0_b;          // wave b phase increment
    float                     w0_sub;        // sub wave phase increment
    float                     lfo;           // target lfo value
    float                     lfoz;          // current interpolated lfo value
    float                     dither;        // dithering amount before bit reduction
    float                     bit_res;       // bit depth scaling factor
    float                     bit_res_recip; // bit depth scaling reciprocal, returns signal to 0.-1.f after scaling/rounding
    float                     imperfection;  // tuning imperfection
    std::atomic_uint_fast32_t flags;         // flags passed to audio processing thread
    
    State(void) :
      wave_a(wavesA[0]),
      wave_b(wavesD[0]),
      sub_wave(wavesA[0]),
      w0_a(440.f * k_samplerate_recipf),
      w0_b(440.f * k_samplerate_recipf),
      w0_sub(220.f * k_samplerate_recipf),
      lfo(0.f),
      lfoz(0.f),
      dither(0.f),
      bit_res(1.f),
      bit_res_recip(1.f),
      flags{k_flags_none}
    {
      Reset();
      imperfection = osc_white() * 1.0417e-006f; // +/- 0.05Hz@48KHz
    }
    
    inline void Reset(void) {
      phi_a = 0;
      phi_b = 0;
      phi_sub = 0;
      lfo = lfoz;
    }
  };


  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/
  
  Waves(void)  { }
  ~Waves(void) { }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (!desc)
      return k_unit_err_undef;
    
    // Note: make sure the unit is being loaded to the correct platform/module target
    if (desc->target != unit_header.target)
      return k_unit_err_target;
    
    // Note: check API compatibility with the one this unit was built against
    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;
    
    // Check compatibility of samplerate with unit, for NTS-1 MKII should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    if (desc->input_channels != 2 || desc->output_channels != 1)  // should be stereo input / mono output
      return k_unit_err_geometry;

    // Initialize pre/post filter coefficients
    prelpf_.mCoeffs.setPoleLP(0.9f);
    postlpf_.mCoeffs.setFOLP(osc_tanpif(0.45f));

    // Cache runtime descriptor to keep access to API hooks
    runtime_desc_ = *desc;
    
    // Make sure parameters are reset to default values
    params_.reset();
    
    return k_unit_err_none;
  }

  inline void Teardown() {
    // Note: cleanup and release resources if any
  }
  
  inline void Reset() {
    // Note: Reset effect state, excluding exposed parameter values.
    state_.Reset();
  }

  inline void Resume() {
    // Unit will resume and exit suspend state.
    // Unit was selected and the render callback will start being called again
    state_.Reset();
  }

  inline void Suspend() {
    // Unit will enter suspend state.
    // Another unit was selected and the render callback has stopped being called
  }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void Process(const float * in, float * out, size_t frames) {
    (void)in; // Note: not using input
    
    State &s = state_;
    const Waves::Params &p = params_;
    const unit_runtime_osc_context_t *ctxt = static_cast<const unit_runtime_osc_context_t *>(runtime_desc_.hooks.runtime_context);
    
    // Handle events.
    {      
      updatePitch(osc_w0f_for_note((ctxt->pitch)>>8, ctxt->pitch & 0xFF));

      const uint32_t flags = s.flags.exchange(State::k_flags_none, std::memory_order_relaxed);
      updateWaves(flags);
      
      if (flags & State::k_flag_reset)
        s.Reset();
      
      s.lfo = q31_to_f32(ctxt->shape_lfo);
      
      if (flags & State::k_flag_bit_crush) {
        s.dither = p.bit_crush * 2e-008f;
        s.bit_res = osc_bitresf(p.bit_crush);
        s.bit_res_recip = 1.f / s.bit_res;
      }
    }
    
    // Temporaries.
    float phi_a = s.phi_a;
    float phi_b = s.phi_b;
    float phi_sub = s.phi_sub;
    
    float lfoz = s.lfoz;
    const float lfo_inc = (s.lfo - lfoz) / frames;
    
    const float ditheramt = p.bit_crush * 2e-008f;
        
    const float sub_mix = p.sub_mix * 0.5011872336272722f;
    const float ring_mix = p.ring_mix;
    
    float * __restrict y = out;
    const float * y_e = y + frames;
  
    for (; y != y_e; ) {
      const float wave_mix = clip01f(p.shape+lfoz);
      
      float sig = (1.f - wave_mix) * osc_wave_scanf(s.wave_a, phi_a);
      sig += wave_mix * osc_wave_scanf(s.wave_b, phi_b);
    
      const float sub_sig = osc_wave_scanf(s.sub_wave, phi_sub);
      sig = (1.f - ring_mix) * sig + ring_mix * 1.4125375446227544f * (sub_sig * sig);
      sig += sub_mix * sub_sig;
      sig *= 1.4125375446227544f;
      sig = clip1m1f(fastertanh2f(sig));
    
      sig = prelpf_.process_fo(sig);
      sig += s.dither * osc_white();
      sig = si_roundf(sig * s.bit_res) * s.bit_res_recip;
      sig = postlpf_.process_fo(sig);
      
      *(y++) = sig;
    
      phi_a += s.w0_a;
      phi_a -= (uint32_t)phi_a;
      phi_b += s.w0_b;
      phi_b -= (uint32_t)phi_b;
      phi_sub += s.w0_sub;
      phi_sub -= (uint32_t)phi_sub;
      lfoz += lfo_inc;
    }

    // Update state
    s.phi_a = phi_a;
    s.phi_b = phi_b;
    s.phi_sub = phi_sub;
    s.lfoz = lfoz;
  }

  inline void setParameter(uint8_t index, int32_t value) {
    Params &p = params_;
    State &s = state_;
    
    switch (index) {
    case Params::k_shape:
      //  min, max,  center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   1023, 0,      0,       k_unit_param_type_none, 0,    0,          0,          {"SHAPE"}},
      p.shape = 0.005f + param_10bit_to_f32(value) * 0.99f;
      break;

    case Params::k_sub_mix:
      //  min, max,  center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   1023, 0,      0,       k_unit_param_type_none, 0,    0,          0,          {"SUB"}},
      p.sub_mix = 0.05f + param_10bit_to_f32(value) * 0.90f;
      break;
            
    case Params::k_wave_a: {
      //  min, max,          center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   WAVE_A_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"WAVE A"}},
      p.wave_a = value % WAVE_A_CNT;
      s.flags.fetch_or(State::k_flag_wave_a);
    }
    break;
    
    case Params::k_wave_b: {
      //  min, max,          center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   WAVE_B_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"WAVE B"}},
      p.wave_b = value % WAVE_B_CNT;
      s.flags.fetch_or(State::k_flag_wave_b);
    }
    break;
    
    case Params::k_sub_wave:
      //  min, max,            center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   SUB_WAVE_CNT-1, 0,      0,       k_unit_param_type_enum, 0,    0,          0,          {"SUB WAVE"}},
      p.sub_wave = value % SUB_WAVE_CNT;
      s.flags.fetch_or(State::k_flag_sub_wave);
      break;
        
    case Params::k_ring_mix:
      //  min, max,  center, default, type,                      frac, frac. mode, <reserved>, name
      // {0,   1000,  0,      0,       k_unit_param_type_percent, 1,    1,          0,          {"RING MIX"}},
      p.ring_mix = clip01f(value * 0.001f);
      break;
    
    case Params::k_bit_crush:
      //  min, max,  center, default, type,                      frac, frac. mode, <reserved>, name
      // {0,   1000,  0,      0,       k_unit_param_type_percent, 1,    1,          0,          {"BIT CRUSH"}},
      p.bit_crush = clip01f(value * 0.001f);
      s.flags.fetch_or(State::k_flag_bit_crush);
      break;

    case Params::k_drift:
      //  min, max,  center, default, type,                      frac, frac. mode, <reserved>, name
      // {0,   1000,  0,      250,     k_unit_param_type_percent, 1,    1,          0,          {"DRIFT"}},
      p.drift = 1.f + value * 0.001f;
      break;
      
    default:
      break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    const Params &p = params_;
    
    switch (index) {
    case Params::k_shape:
      //  min, max,  center, default, type,                   frac, frac. mode, <reserved>, name
      // {0,   1023, 0,      0,       k_unit_param_type_none, 0,    0,          0,          {"SHAPE"}},
      return param_f32_to_10bit((p.shape - 0.05) / 0.99f);

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

    return INT_MIN; // Note: will be handled as invalid
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    (void)value;
    switch (index) {
      // Note: String memory must be accessible even after function returned.
      //       It can be assumed that caller will have copied or used the string
      //       before the next call to getParameterStrValue
      
      // Currently no Parameters of type k_unit_param_type_strings.

    default:
      break;
    }
    return nullptr;
  }

  inline void NoteOn(uint8_t note, uint8_t velo) {
    (void)velo;
    
    // Schedule phase reset
    state_.flags.fetch_or(State::k_flag_reset);
    
    // TODO: should we still fully rely on osc context pitch?
  }

  inline void NoteOff(uint8_t note) {
    (uint8_t)note;
  }

  inline void AllNoteOff() {
  }

  inline void PitchBend(uint8_t bend) {
    (uint8_t)bend;
  }

  inline void ChannelPressure(uint8_t press) {
    (uint8_t)press;
  }

  inline void AfterTouch(uint8_t note, uint8_t press) {
    (uint8_t)note;
    (uint8_t)press;
  }

  
  /*===========================================================================*/
  /* Static Members. */
  /*===========================================================================*/
  
 private:
  /*===========================================================================*/
  /* Private Member Variables. */
  /*===========================================================================*/
  
  State       state_;
  Params      params_;
  dsp::BiQuad prelpf_, postlpf_;
  unit_runtime_desc_t runtime_desc_;
  
  /*===========================================================================*/
  /* Private Methods. */
  /*===========================================================================*/

  fast_inline void updatePitch(float w0) {
    w0 += state_.imperfection;
    const float drift = params_.drift;
    state_.w0_a = w0;
    // Alt. osc with slight phase drift (0.25Hz@48KHz)
    state_.w0_b = w0 + drift * 5.20833333333333e-006f;
    // Sub one octave down, with a phase drift (0.15Hz@48KHz)
    state_.w0_sub = 0.5f * w0 + drift * 3.125e-006f;
  }
    
  fast_inline void updateWaves(const uint32_t flags) {
    if (flags & State::k_flag_wave_a) {
      static const uint8_t k_a_thr = k_waves_a_cnt;
      static const uint8_t k_b_thr = k_a_thr + k_waves_b_cnt;
      static const uint8_t k_c_thr = k_b_thr + k_waves_c_cnt;
      
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
      static const uint8_t k_f_thr = k_e_thr + k_waves_f_cnt;
      
      uint8_t idx = params_.wave_b;
      const float * const * table;
      
      if (idx < k_d_thr) {
        table = wavesD;
      }
      else if (idx < k_e_thr) {
        table = wavesE;
        idx -= k_d_thr;
      }
      else { // if (idx < k_f_thr) {
        table = wavesF;
        idx -= k_e_thr;
      }
      
      state_.wave_b = table[idx];
    }
    if (flags & State::k_flag_sub_wave) {
      const uint8_t idx = params_.sub_wave;
      state_.sub_wave = wavesA[params_.sub_wave];
    }
  }
  
  /*===========================================================================*/
  /* Constants. */
  /*===========================================================================*/
  
};
