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

/*
 * File: waves.cpp
 *
 * Morphing wavetable oscillator
 *
 */

#include "userosc.h"
#include "waves.hpp"
typedef __uint32_t uint32_t; // needed for VSCode

static Waves s_waves;

// init event to initialize an oscillator during load phase this user OSC
void OSC_INIT(uint32_t platform, uint32_t api)
{
  // create the struct
  // hopefully the synth is dealing with the clean-up correctly...
  s_waves = Waves();
}

// cycle event, called (48k divided by number frames) times per second
// buffer processing, buffer at yn must be filled by #frames
void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  
  // dereference some state and parameters for easy access and performance
  Waves::State &s = s_waves.state;
  const Waves::Params &p = s_waves.params;
  // watch out! p is our struct's parameters, while params is an argument of this function

  // Handle events.
  {
    // temp copy of flags
    const uint32_t flags = s.flags;
    // clear flags for next cycle
    s.flags = Waves::k_flags_none;
    
    // handle any changes in pitch parameters (does this include pitch LFO and EG? probably yes)
    // params->pitch is note in  in upper byte in [0-151] range, 
    // mod is in lower byte in [0-255] range.
    s_waves.updatePitch(osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF));
    
    // if any wave parameters have changed, update waves accordingly
    // wave change requests are specifed in the flags
    s_waves.updateWaves(flags);
    
    // reset oscillator phases (phase accumulators) to 0
    if (flags & Waves::k_flag_reset)
      s.reset();
    
    // q31 is the fixed point ratio in 31 bits (32nd bit used for sign)
    // this is the value of the LFO when it targets the SHAPE
    s.lfo = q31_to_f32(params->shape_lfo);

    // compute bitcrush amount if changed in the program edit
    if (flags & Waves::k_flag_bitcrush) {
      s.dither = p.bitcrush * 2e-008f;
      s.bitres = osc_bitresf(p.bitcrush);
      s.bitresrcp = 1.f / s.bitres; 
      // rcp stands for reciprocal
    }
  }
  
  // Temporaries.
  // current phase of wave 1
  float phi0 = s.phi0;
  // current phase of wave 1
  float phi1 = s.phi1;      
  // current phase of sub wave
  float phisub = s.phisub;  

  // lfo phase
  float lfoz = s.lfoz;      
  // lfo increment per sample
  const float lfo_inc = (s.lfo - lfoz) / frames;  
  
  const float ditheramt = p.bitcrush * 2e-008f;
  
  const float bitres = osc_bitresf(p.bitcrush);
  // recip stands for reciprocal
  const float bitres_recip = 1.f / bitres; 

  // mix amount of sub oscillator from the program edit
  const float submix = p.submix;    
  // amount of ring modulation from the program edit
  const float ringmix = p.ringmix;  
  
  // pre filter for bit crushing
  dsp::BiQuad &prelpf = s_waves.prelpf;   
  // pre filter for bit crushing
  dsp::BiQuad &postlpf = s_waves.postlpf; 
  
  // buffer start is at yn
  // buffer position is at y
  q31_t * __restrict y = (q31_t *)yn;
  // buffer end is at y_e
  const q31_t * y_e = y + frames;
  
  // loop until you have reached end of buffer y_e
  for (; y != y_e; ) {

    // SHAPE is the blend between wave 1 and wave 2 like a DJ fader
    // LFO oscillates around that shape balance
    // why is there a 0.5% mix leftover?
    const float wavemix = clipminmaxf(0.005f, p.shape+lfoz, 0.995f);
    
    // get wave 1 and attenuate it
    float sig = (1.f - wavemix) * osc_wave_scanf(s.wave0, phi0);
    // get wave 2 and mix it to sig
    sig += wavemix * osc_wave_scanf(s.wave1, phi1);
    
    // get sub wave and mix it to sig
    const float subsig = osc_wave_scanf(s.subwave, phisub);
    // note: submix param decreases volume of wave 1 and 2
    sig = (1.f - submix) * sig + submix * subsig;
    // apply ring modulation
    sig = (1.f - ringmix) * sig + ringmix * (subsig * sig);
    // apply clipping
    sig = clip1m1f(sig);
    
    // do some bit crush magic: dithering, filtering, clipping
    sig = prelpf.process_fo(sig);
    sig += s.dither * osc_white();
    sig = si_roundf(sig * s.bitres) * s.bitresrcp;
    sig = postlpf.process_fo(sig);
    sig = osc_softclipf(0.125f, sig);
    
    // write signal to buffer and advance position y
    *(y++) = f32_to_q31(sig);
    
    // add phase increments to phase accumulators
    // subtract any integer value, essentially: do modulo 1
    phi0 += s.w00;
    phi0 -= (uint32_t)phi0;
    phi1 += s.w01;
    phi1 -= (uint32_t)phi1;
    phisub += s.w0sub;
    phisub -= (uint32_t)phisub;
    // dont do modulo 1 for the lfo, why?
    lfoz += lfo_inc;
  }
  
  // store wave phases accumulators in OSC state (struct)
  s.phi0 = phi0;
  s.phi1 = phi1;
  s.phisub = phisub;
  s.lfoz = lfoz;
}

// note on event, so reset the wave phases to 0
void OSC_NOTEON(const user_osc_param_t * const params)
{
  // change the flag to reset the waves at next cycle, but keep other state changes (flags)
  s_waves.state.flags |= Waves::k_flag_reset;
}

// note off event
void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  // says to compiler: hey, I won't use params, but don't warn me
  // basically this is a NOOP
  (void)params;
}

// parameter change event
void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  Waves::Params &p = s_waves.params;
  Waves::State &s = s_waves.state;
  
  // which parameter has changed
  switch (index) {
    // some cases change the corresponding Params parameter after normalizing the parameters
    // others additionally set the corresponding flag in the state to indicate a change for OSC_CYCLE
  case k_osc_param_id1:
    // wave 0
    // select parameter
    {
      // user changed program edit parameter Wave 1
      static const uint8_t cnt = k_waves_a_cnt + k_waves_b_cnt + k_waves_c_cnt; 
      p.wave0 = value % cnt;
      // remember to change the wave during OSC_CYCLE
      s.flags |= Waves::k_flag_wave0;
    }
    break;
    
  case k_osc_param_id2:
    // wave 1
    // select parameter
    {
      // user changed program edit parameter Wave 2
      static const uint8_t cnt = k_waves_d_cnt + k_waves_e_cnt + k_waves_f_cnt; 
      p.wave1 = value % cnt;
      // remember to change the wave during OSC_CYCLE
      s.flags |= Waves::k_flag_wave1;
    }
    break;
    
  case k_osc_param_id3:
    // sub wave
    // select parameter
    // user changed program edit parameter Sub Wave
    p.subwave = value % k_waves_a_cnt;
    s.flags |= Waves::k_flag_subwave;
    break;
    
  case k_osc_param_id4:
    // sub mix
    // percent parameter
    // user changed program edit Sub Mix
    p.submix = clip01f(0.05f + value * 0.01f * 0.90f); // scale in 0.05 - 0.95
    break;
    
  case k_osc_param_id5:
    // ring mix
    // percent parameter
    // user changed program edit parameter Ring Mix
    p.ringmix = clip01f(value * 0.01f);
    break;
    
  case k_osc_param_id6:
    // bit crush
    // percent parameter
    // user changed program edit parameter Bit Crush
    p.bitcrush = clip01f(value * 0.01f);
    s.flags |= Waves::k_flag_bitcrush;
    break;
    
  case k_osc_param_shape:
    // 10bit parameter
    // user changed MULTI ENGINE shape knob
    // this is used as a blend factor between wave 1 and wave 2
    p.shape = param_val_to_f32(value);
    break;
    
  case k_osc_param_shiftshape:
    // 10bit parameter
    // user changed MULTI ENGINE shape knob while pressing shift
    // this is used as the drift amount of wave 1, wave 2 and sub wave
    p.shiftshape = 1.f + param_val_to_f32(value); 
    break;
    
  default:
    break;
  }
}

