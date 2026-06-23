#pragma once
/*
 *  File: synth.h
 *
 *  "SmplVox" - a sample-playback drum voice for the drumlogue.
 *
 *  This unit is built specifically around capabilities that are unique to the
 *  drumlogue within the logue-SDK family:
 *
 *    1. On-board PCM sample banks. The runtime descriptor hands user units a set
 *       of accessor function pointers (get_num_sample_banks /
 *       get_num_samples_for_bank / get_sample) that expose every factory and
 *       user sample loaded on the device as a sample_wrapper_t (interleaved
 *       float PCM, with name / channel-count / frame-count metadata). No other
 *       logue platform lets custom code read the device's sample memory.
 *
 *    2. A Linux/Cortex-A7 runtime. Unlike the bare-metal gen-1/gen-2 MCU
 *       targets, drumlogue units are ordinary shared objects, so we can lean on
 *       the C++ standard library and ordinary floating point freely.
 *
 *    3. Gate-based drum triggering with per-hit velocity, driven by the
 *       drumlogue's step sequencer (unit_gate_on / unit_gate_off).
 *
 *  The voice resamples the selected sample with linear interpolation (so it can
 *  be pitched/tuned/reversed), then shapes it with the kind of controls you want
 *  on a drum sound: a decay envelope, a pitch-sweep envelope for punch, bit
 *  reduction for lo-fi grit, and drive for saturation.
 *
 *  2024 (c) Korg
 *
 */

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "unit.h"  // Note: Include common definitions for all units

class Synth {
 public:
  /*===========================================================================*/
  /* Parameter indices (must match header.c). */
  /*===========================================================================*/

  enum {
    kParamBank = 0,
    kParamSample,
    kParamPitch,
    kParamFine,
    kParamDecay,
    kParamPSweep,
    kParamPTime,
    kParamStart,
    kParamReverse,
    kParamCrush,
    kParamDrive,
    kParamPan,
    kNumParams,
  };

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Synth(void) {}
  ~Synth(void) {}

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    if (!desc)
      return k_unit_err_undef;

    // drumlogue always runs at 48kHz / stereo out; reject anything else.
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;
    if (desc->output_channels != 2)
      return k_unit_err_geometry;

    // The sample-bank accessors are the whole point of this unit, so make sure
    // the host actually provided them before accepting initialization.
    if (desc->get_num_sample_banks == nullptr ||
        desc->get_num_samples_for_bank == nullptr ||
        desc->get_sample == nullptr)
      return k_unit_err_undef;

    desc_ = *desc;
    sr_ = static_cast<float>(desc->samplerate);

    for (uint8_t i = 0; i < kNumParams; ++i)
      params_[i] = 0;
    params_[kParamDecay] = (60 << 1);
    params_[kParamPTime] = (20 << 1);

    num_banks_ = desc_.get_num_sample_banks();

    resolveSample();
    return k_unit_err_none;
  }

  inline void Teardown() {}

  inline void Reset() {
    active_ = false;
    amp_env_ = 0.f;
  }

  inline void Resume() {}
  inline void Suspend() { active_ = false; }

  /*===========================================================================*/
  /* Render. */
  /*===========================================================================*/

  fast_inline void Render(float * out, size_t frames) {
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1);  // stereo interleaved

    const sample_wrapper_t * s = sample_;

    if (!active_ || s == nullptr || s->sample_ptr == nullptr || s->frames < 2) {
      for (; out_p != out_e; out_p += 2) {
        out_p[0] = 0.f;
        out_p[1] = 0.f;
      }
      return;
    }

    const float * data = s->sample_ptr;
    const uint8_t channels = s->channels;
    const int32_t last_frame = static_cast<int32_t>(s->frames) - 1;
    const float dir = reverse_ ? -1.f : 1.f;

    for (; out_p != out_e; out_p += 2) {
      if (!active_) {
        out_p[0] = 0.f;
        out_p[1] = 0.f;
        continue;
      }

      // --- Linear interpolation read --------------------------------------
      int32_t i0 = static_cast<int32_t>(pos_);
      if (i0 < 0) i0 = 0;
      if (i0 > last_frame - 1) i0 = last_frame - 1;
      const float t = pos_ - static_cast<float>(i0);

      float l, r;
      if (channels >= 2) {
        const float * f0 = data + (static_cast<size_t>(i0) << 1);
        l = f0[0] + t * (f0[2] - f0[0]);
        r = f0[1] + t * (f0[3] - f0[1]);
      } else {
        const float * f0 = data + i0;
        const float m = f0[0] + t * (f0[1] - f0[0]);
        l = m;
        r = m;
      }

      // --- Amp envelope * velocity ----------------------------------------
      const float a = amp_env_ * vel_amp_;
      l *= a;
      r *= a;

      // --- Bit-depth reduction (lo-fi grit) -------------------------------
      if (crush_levels_ > 0.f) {
        l = std::round(l * crush_levels_) * crush_inv_;
        r = std::round(r * crush_levels_) * crush_inv_;
      }

      // --- Drive / saturation ---------------------------------------------
      if (drive_pre_ > 1.f) {
        l = std::tanh(l * drive_pre_);
        r = std::tanh(r * drive_pre_);
      }

      // --- Pan & output ---------------------------------------------------
      out_p[0] = l * pan_l_;
      out_p[1] = r * pan_r_;

      // --- Advance playhead -----------------------------------------------
      const float semis = base_semis_ + sweep_semis_ * pitch_env_;
      const float speed = exp2f(semis * (1.f / 12.f));
      pos_ += dir * speed;

      // --- Update envelopes -----------------------------------------------
      amp_env_ *= amp_coef_;
      pitch_env_ *= pitch_coef_;

      // --- End conditions -------------------------------------------------
      if (amp_env_ < 1e-4f ||
          (!reverse_ && pos_ >= static_cast<float>(last_frame)) ||
          (reverse_ && pos_ <= 0.f)) {
        active_ = false;
      }
    }
  }

  /*===========================================================================*/
  /* Parameters. */
  /*===========================================================================*/

  inline void setParameter(uint8_t index, int32_t value) {
    if (index >= kNumParams)
      return;
    params_[index] = value;

    switch (index) {
      case kParamBank:
      case kParamSample:
        resolveSample();
        break;
      case kParamCrush:
        updateCrush();
        break;
      case kParamDrive:
        updateDrive();
        break;
      case kParamPan:
        updatePan();
        break;
      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    if (index >= kNumParams)
      return 0;
    return params_[index];
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    switch (index) {
      case kParamBank: {
        static char buf[8];
        const uint8_t b = clampBank(static_cast<int32_t>(value));
        buf[0] = 'B';
        buf[1] = 'K';
        buf[2] = ' ';
        // value is 0-based; display 1-based, supports up to 2 digits.
        const int n = b + 1;
        if (n >= 10) {
          buf[3] = '0' + (n / 10);
          buf[4] = '0' + (n % 10);
          buf[5] = '\0';
        } else {
          buf[3] = '0' + n;
          buf[4] = '\0';
        }
        return buf;
      }
      case kParamSample: {
        const uint8_t bank = clampBank(params_[kParamBank]);
        const uint8_t count = desc_.get_num_samples_for_bank(bank);
        if (count == 0)
          return "---";
        uint8_t si = (value < 0) ? 0 : static_cast<uint8_t>(value);
        if (si >= count)
          si = count - 1;
        const sample_wrapper_t * w = desc_.get_sample(bank, si);
        if (w == nullptr)
          return "---";
        return w->name;
      }
      default:
        break;
    }
    return nullptr;
  }

  inline const uint8_t * getParameterBmpValue(uint8_t index, int32_t value) const {
    (void)index;
    (void)value;
    return nullptr;
  }

  /*===========================================================================*/
  /* Triggering. */
  /*===========================================================================*/

  inline void NoteOn(uint8_t note, uint8_t velocity) {
    note_ = note;
    trigger(velocity);
  }

  inline void NoteOff(uint8_t note) { (void)note; }

  inline void GateOn(uint8_t velocity) { trigger(velocity); }

  inline void GateOff() {}

  inline void AllNoteOff() { active_ = false; }

  inline void PitchBend(uint16_t bend) { (void)bend; }
  inline void ChannelPressure(uint8_t pressure) { (void)pressure; }
  inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
    (void)note;
    (void)aftertouch;
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }
  inline uint8_t getPresetIndex() const { return 0; }
  static inline const char * getPresetName(uint8_t idx) {
    (void)idx;
    return nullptr;
  }

 private:
  /*===========================================================================*/
  /* Helpers. */
  /*===========================================================================*/

  inline uint8_t clampBank(int32_t bank) const {
    if (num_banks_ == 0)
      return 0;
    if (bank < 0)
      return 0;
    if (bank >= num_banks_)
      return num_banks_ - 1;
    return static_cast<uint8_t>(bank);
  }

  // Resolve the currently selected (bank, sample) into a sample wrapper pointer.
  // Called from the UI thread on parameter changes; the audio thread only ever
  // reads the cached pointer.
  inline void resolveSample() {
    if (desc_.get_sample == nullptr) {
      sample_ = nullptr;
      return;
    }
    const uint8_t bank = clampBank(params_[kParamBank]);
    const uint8_t count = desc_.get_num_samples_for_bank(bank);
    if (count == 0) {
      sample_ = nullptr;
      return;
    }
    int32_t si = params_[kParamSample];
    if (si < 0)
      si = 0;
    if (si >= count)
      si = count - 1;
    sample_ = desc_.get_sample(bank, static_cast<uint8_t>(si));
  }

  inline void updateCrush() {
    const float n = params_[kParamCrush] * (1.f / 200.f);  // 0..1
    if (n <= 0.f) {
      crush_levels_ = 0.f;  // disabled
      return;
    }
    // 16 bits (clean) down to ~2 bits (heavy).
    const float bits = 16.f - n * 14.f;
    crush_levels_ = exp2f(bits - 1.f);  // quantization levels per polarity
    crush_inv_ = 1.f / crush_levels_;
  }

  inline void updateDrive() {
    const float n = params_[kParamDrive] * (1.f / 200.f);  // 0..1
    drive_pre_ = 1.f + n * 23.f;                           // 1x .. 24x pre-gain
  }

  inline void updatePan() {
    // Equal-power pan from -100..100.
    const float p = (params_[kParamPan] * 0.005f) * 0.5f + 0.5f;  // 0..1
    pan_l_ = std::cos(p * 1.5707963f);
    pan_r_ = std::sin(p * 1.5707963f);
  }

  inline void trigger(uint8_t velocity) {
    resolveSample();
    if (sample_ == nullptr || sample_->sample_ptr == nullptr || sample_->frames < 2)
      return;

    reverse_ = (params_[kParamReverse] != 0);

    // Velocity: scale amplitude, mild curve for a more natural response.
    const float v = velocity * (1.f / 127.f);
    vel_amp_ = v * v;

    // Base pitch in semitones: note offset from C3(60) + coarse + fine.
    base_semis_ = static_cast<float>(params_[kParamPitch]) +
                  params_[kParamFine] * 0.01f +
                  (static_cast<float>(note_) - 60.f);

    // Pitch sweep: start offset (semitones) that decays to zero.
    sweep_semis_ = static_cast<float>(params_[kParamPSweep]);
    pitch_env_ = 1.f;

    // Envelope coefficients (per-frame exponential decay).
    const float decay_n = params_[kParamDecay] * (1.f / 200.f);
    const float decay_sec = 0.002f * powf(2000.f, decay_n);  // ~2ms..4s
    amp_coef_ = expf(-1.f / (decay_sec * sr_));
    amp_env_ = 1.f;

    const float ptime_n = params_[kParamPTime] * (1.f / 200.f);
    const float ptime_sec = 0.002f * powf(500.f, ptime_n);  // ~2ms..1s
    pitch_coef_ = expf(-1.f / (ptime_sec * sr_));

    // Start position from the START offset.
    const float start_n = params_[kParamStart] * (1.f / 200.f);
    const int32_t last_frame = static_cast<int32_t>(sample_->frames) - 1;
    if (reverse_)
      pos_ = static_cast<float>(last_frame) - start_n * last_frame;
    else
      pos_ = start_n * last_frame;

    updateCrush();
    updateDrive();
    updatePan();

    active_ = true;
  }

  /*===========================================================================*/
  /* Member Variables. */
  /*===========================================================================*/

  unit_runtime_desc_t desc_ = {};
  float sr_ = 48000.f;
  uint8_t num_banks_ = 0;

  int32_t params_[kNumParams] = {0};

  // Cached selected sample (written on UI thread, read on audio thread).
  const sample_wrapper_t * sample_ = nullptr;

  // Per-hit voice state.
  std::atomic<bool> active_{false};
  bool reverse_ = false;
  uint8_t note_ = 60;
  float pos_ = 0.f;
  float vel_amp_ = 1.f;

  float base_semis_ = 0.f;
  float sweep_semis_ = 0.f;

  float amp_env_ = 0.f;
  float amp_coef_ = 0.f;
  float pitch_env_ = 0.f;
  float pitch_coef_ = 0.f;

  float crush_levels_ = 0.f;  // 0 => disabled
  float crush_inv_ = 1.f;
  float drive_pre_ = 1.f;     // <=1 => disabled

  float pan_l_ = 0.70710678f;
  float pan_r_ = 0.70710678f;
};
