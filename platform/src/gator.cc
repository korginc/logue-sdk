/*
 *  File: gator.cc
 *
 *  Gator 2.0 unit
 *
 *  2020-2024 (c) Oleg Burdaev
 *  mailto: dukesrg@gmail.com
 */

#include "logue_wrap.h"
#include "frac_value.h"

#include <cstddef>
#include <cstdint>

#include <ctype.h>
#include <limits.h>

#ifdef UNIT_TARGET_PLATFORM_DRUMLOGUE
#include <arm_neon.h>
#endif

#ifdef UNIT_OSC_H_
#define VELOCITY_SENSITIVITY .5f
#define VELOCITY_LOW -144.f
#endif

#include "peak_detect.h"
#include "gate_arp.h"
#include "envelopes.h"

enum {
  param_arp_gate = 
#ifdef UNIT_OSC_H_
  k_num_unit_osc_fixed_param_id
#elif defined(UNIT_MODFX_H_)
  k_num_unit_modfx_fixed_param_id
#elif defined(UNIT_DELFX_H_)
  k_num_unit_delfx_fixed_param_id
#elif defined(UNIT_REVFX_H_)
  k_num_unit_revfx_fixed_param_id
#elif defined(UNIT_GENERICFX_H_)
  k_num_unit_genericfx_fixed_param_id
#else
  0U
#endif
  ,
  param_arp_pattern,
  param_peak_threshold,
  param_peak_time,
  param_eg_attack,
  param_eg_decay,
  param_eg_sustain,
  param_eg_release,
#if (defined(UNIT_TARGET_PLATFORM_DRUMLOGUE) && defined(UNIT_TARGET_MODULE_MASTERFX))
  param_master,
  param_sidechain,
  param_peak_source,
#endif
  param_num
};  

#if (defined(UNIT_TARGET_PLATFORM_DRUMLOGUE) && defined(UNIT_TARGET_MODULE_MASTERFX))
enum {
  route_off = 0U,
  route_on,
  route_arp
};

enum {
  source_master = 0U,
  source_sidechain,
  source_both
};

static uint32_t MasterRoute[2];
#endif
static uint32_t PeakSourceMask = 0x0F;

static int32_t Params[PARAM_COUNT];

static eg_adsr EG;

#ifdef UNIT_OSC_H_
static float Sustain;
static float Velocity = VELOCITY_LOW;

fast_inline void set_velocity(float velocity) {
  Velocity = (velocity - 127.f) * VELOCITY_SENSITIVITY;
  EG.set_sustain(Velocity > Sustain ? Velocity : Sustain);
}

fast_inline void set_sustain(float sustain) {
  Sustain = sustain;
  EG.set_sustain(Velocity > Sustain ? Velocity : Sustain);
}
#else
fast_inline void set_sustain(float sustain) {
  EG.set_sustain(sustain);
}
#endif

fast_inline void arp_gate_on_callback() {
  EG.start();
}

fast_inline void arp_gate_off_callback() {
  EG.stop();
#ifdef UNIT_OSC_H_
  set_velocity(VELOCITY_LOW);
#endif
}

static gate_arp ARP(&arp_gate_on_callback, &arp_gate_off_callback);

fast_inline void peak_high_callback() {
  ARP.start();
}

fast_inline void peak_low_callback() {
  ARP.stop();
}

static peak_detect Peak(&peak_high_callback, &peak_low_callback);

#ifdef UNIT_OSC_H_
unit_runtime_osc_context_t *runtime_context;
#endif

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc) {
  if (!desc)
    return k_unit_err_undef;
  if (desc->target != UNIT_HEADER_TARGET_FIELD)
    return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;
  if (desc->samplerate != 48000)
    return k_unit_err_samplerate;
  if (desc->input_channels != UNIT_INPUT_CHANNELS || desc->output_channels != UNIT_OUTPUT_CHANNELS)
    return k_unit_err_geometry;
#ifdef UNIT_OSC_H_
  runtime_context = (unit_runtime_osc_context_t *)desc->hooks.runtime_context;
  runtime_context->notify_input_usage(k_runtime_osc_input_used);
#endif
  return k_unit_err_none;
}

__unit_callback void unit_reset() {
  arp_gate_off_callback();
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames) {
  float amp = EG.out();
#ifdef UNIT_OSC_H_
  amp *= 1.f - q31_to_f32(runtime_context->shape_lfo);
#elif defined(UNIT_TARGET_PLATFORM_DRUMLOGUE) && defined (UNIT_TARGET_MODULE_MASTERFX)
  float master_amp = MasterRoute[0] == route_arp ? amp : MasterRoute[0]; 
  float sidechain_amp = MasterRoute[1] == route_arp ? amp : MasterRoute[1]; 
  float32x4_t v_amp = {master_amp, master_amp, sidechain_amp, sidechain_amp};
#endif
  const float * __restrict in_p = in;
  float * __restrict out_p = out;
  const float * out_e = out_p + frames * UNIT_OUTPUT_CHANNELS;  
  for (; out_p != out_e; in_p += UNIT_INPUT_CHANNELS, out_p += UNIT_OUTPUT_CHANNELS) {
#ifdef UNIT_TARGET_PLATFORM_DRUMLOGUE
#ifdef UNIT_TARGET_MODULE_MASTERFX
    float32x4_t v_in = vld1q_f32(in_p) * v_amp;
    vst1_f32(out_p, vadd_f32(vget_low_f32(v_in), vget_high_f32(v_in)));
#else
    vst1_f32(out_p, vld1_f32(in_p) * amp);
#endif
#else
#if UNIT_OUTPUT_CHANNELS == 1
    *out_p = (in_p[0] + in_p[1]) * .5f * amp;
#elif UNIT_OUTPUT_CHANNELS == 2
    out_p[0] = in_p[0] * amp;
    out_p[1] = in_p[1] * amp;
#endif
#endif
  }
  EG.advance(frames);
  ARP.advance(frames);
  Peak.process(in, frames, UNIT_INPUT_CHANNELS, PeakSourceMask);
}

__unit_callback void unit_set_param_value(uint8_t index, int32_t value) {
  value = (int16_t)value;
#ifdef UNIT_TARGET_PLATFORM_NTS1_MKII
  if (index == 0) {
    Params[index] = value;
    index = param_arp_gate;
  }
#endif
  Params[index] = value;
  switch (index) {
    case param_arp_gate:
      if (value)
        ARP.start();
      else
        ARP.stop();
      break;
    case param_arp_pattern:
      ARP.set_pattern(value);
      break;
    case param_peak_threshold:
      Peak.set_threshold(value * .1f);
      break;
    case param_peak_time:
      Peak.set_time(value * .001f);
      break;
    case param_eg_attack:
      EG.set_attack(value * .001f);
      break;
    case param_eg_decay:
      EG.set_decay(value * .001f);
      break;
    case param_eg_sustain:
      set_sustain(value * .1f);
      break;
    case param_eg_release:
      EG.set_release(value * .001f);
      break;
#if (defined(UNIT_TARGET_PLATFORM_DRUMLOGUE) && defined(UNIT_TARGET_MODULE_MASTERFX))
    case param_master:
    case param_sidechain:
      MasterRoute[index - param_master] = value;    
      break;
    case param_peak_source:
      switch (value) {
        case source_master:
          PeakSourceMask = 0x03;
          break;
        case source_sidechain:
          PeakSourceMask = 0x0C;
          break;
        case source_both:
          PeakSourceMask = 0x0F;
          break;
      }
      break;
#endif
    default:
      break;
  }
}

__unit_callback int32_t unit_get_param_value(uint8_t index) {
#ifdef UNIT_TARGET_PLATFORM_NTS1_MKII
  if (index == 0)
    index = param_arp_gate;
#endif
  return Params[index];
}

__unit_callback const char * unit_get_param_str_value(uint8_t index, int32_t value) {
//  return unit_get_param_frac_value(index, value);
  value = (int16_t)value;
  switch (index) {
#if (defined(UNIT_TARGET_PLATFORM_DRUMLOGUE) && defined(UNIT_TARGET_MODULE_MASTERFX))
    static const char master_route[][4] = {"OFF", "ON", "ARP"};
    static const char peak_source[][7] = {"MASTER", "SIDECH", "BOTH"};
    case param_master:
    case param_sidechain:
      return master_route[value];
    case param_peak_source:
      return peak_source[value];
#endif
    default:
      break;
  }
  return nullptr;
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
  ARP.set_tempo(tempo);
}

#if defined(UNIT_TARGET_PLATFORM_NTS1_MKII) || defined(UNIT_TARGET_PLATFORM_NTS3_KAOSS)
__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
  ARP.set_tempo_4ppqn_tick(counter);
};
__unit_callback void unit_resume() {}

__unit_callback void unit_suspend() {}
#endif

#ifdef UNIT_OSC_H_
__unit_callback void unit_note_on(uint8_t note, uint8_t velocity) {
  (void)note;
  set_velocity(velocity);
  ARP.start();
}

__unit_callback void unit_note_off(uint8_t note) {
  (void)note;
  ARP.stop();
}

__unit_callback void unit_all_note_off() {
  ARP.stop();
}

__unit_callback void unit_channel_pressure(uint8_t pressure) {
  set_velocity(pressure);
}

__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch) {
  (void)note;
  set_velocity(aftertouch);
}
#endif

#ifdef UNIT_TARGET_PLATFORM_NTS3_KAOSS
__unit_callback void unit_touch_event(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
  (void)id;
  (void)x;
  (void)y;
  switch (phase) {
    case k_unit_touch_phase_began:
      ARP.start();
      break;
    case k_unit_touch_phase_ended:
    case k_unit_touch_phase_cancelled:
      ARP.stop();
      break;  
    default:
      break;
  }
}
#endif
