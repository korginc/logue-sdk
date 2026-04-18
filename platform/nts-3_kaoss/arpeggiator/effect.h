#pragma once
/*
 *  File: effect.h
 *
 *  Arpeggiator logic for NTS-3 kaoss pad kit
 *
 */

#include <atomic>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <vector>

#include "osc_api.h"
#include "unit_genericfx.h"
#include "utils/float_math.h"

class Arpeggiator {
public:
  enum { ROOT = 0U, CHORD, GATE, PATTERN, MODE, RANGE, WAVE, NUM_PARAMS };
  enum { SAW = 0, SINE, SQUARE };
  enum { UP = 0, DOWN, UPDOWN, RANDOM, SEQ };
  
  // Rhythmic divisions (in ticks, where 1 quarter note = 480 logical ticks for high resolution)
  // Higher resolution allows for perfect triplets (480 is divisible by 2, 3, 4, 5)
  enum Pattern { 
    P_1_4 = 0,   // 1/4 note
    P_1_4T,      // 1/4 triplet
    P_1_8,       // 1/8 note
    P_1_8T,      // 1/8 triplet
    P_1_16,      // 1/16 note
    P_1_16T,     // 1/16 triplet
    P_1_32,      // 1/32 note
    P_1_32T      // 1/32 triplet
  };

  struct Params {
    uint32_t root{60};      // MIDI note
    uint8_t chord{0};       // Chord index
    float gate{0.5f};       // Gate length (0-1)
    uint8_t pattern{P_1_8}; // Rhythmic pattern
    uint8_t mode{UP};       // Playback mode
    uint8_t range{1};       // Octave range
    uint8_t wave{SAW};      // Oscillator type

    void reset() {
      root = 60;
      chord = 0;
      gate = 0.5f;
      pattern = P_1_8;
      mode = UP;
      range = 1;
      wave = SAW;
    }
  };

  Arpeggiator(void) {
    active_notes_count_ = 0;
  }
  
  ~Arpeggiator(void) {}

  inline int8_t Init(const unit_runtime_desc_t *desc) {
    if (!desc) return k_unit_err_undef;
    if (desc->target != (UNIT_TARGET_PLATFORM | k_unit_module_genericfx)) return k_unit_err_target;
    if (!UNIT_API_IS_COMPAT(desc->api)) return k_unit_err_api_version;
    if (desc->samplerate != 48000) return k_unit_err_samplerate;

    samplerate_ = desc->samplerate;
    params_.reset();
    reset_state();
    setTempo(120 << 8); // 120 BPM default
    updateActiveNotes();

    return k_unit_err_none;
  }

  inline void Teardown() {}

  inline void Reset() { reset_state(); }
  inline void Resume() { is_active_ = true; }
  inline void Suspend() { is_active_ = false; }

  inline void reset_state() {
    is_active_ = true;
    is_touched_ = false;
    phase_ = 0.f;
    amp_ = 0.f;
    target_amp_ = 0.f;
    current_note_hz_ = 0.f;
    smooth_hz_ = 0.f;
    
    seq_index_ = 0;
    seq_dir_ = 1;
    
    samples_per_tick_accum_ = 0.f;
  }

  // BPM is in 1/256 units
  inline void setTempo(uint32_t tempo) {
    last_tempo_ = tempo;
    float bpm = (float)tempo / 256.0f;
    if (bpm < 1.0f) bpm = 1.0f;
    
    // Calculate samples per 1/4 note
    float samples_per_beat = (60.0f / bpm) * samplerate_;
    
    // We use a "logical tick" resolution of 1/480 per quarter note.
    // However, for simplicity and stability, we can just calculate 
    // the period for each pattern division directly.
    updatePatternPeriod(samples_per_beat);
  }

  inline void updatePatternPeriod(float samples_per_beat) {
    static const float divisions[] = {
      1.0f,         // 1/4
      2.0f/3.0f,    // 1/4T
      0.5f,         // 1/8
      1.0f/3.0f,    // 1/8T
      0.25f,        // 1/16
      1.0f/6.0f,    // 1/16T
      0.125f,       // 1/32
      1.0f/12.0f    // 1/32T
    };
    
    samples_per_step_ = samples_per_beat * divisions[params_.pattern & 7];
  }

  inline void tempoTick(uint32_t counter) {
    // Not strictly needed if we use sample-accurate timing, 
    // but can be used for sync reset if desired.
    (void)counter;
  }

  fast_inline void Process(const float *in, float *out, size_t frames) {
    const float *__restrict in_p = in;
    float *__restrict out_p = out;
    const float *out_e = out_p + (frames << 1);

    for (; out_p != out_e; in_p += 2, out_p += 2) {
      if (is_touched_) {
        // Step the arpeggiator
        samples_per_tick_accum_ += 1.0f;
        if (samples_per_tick_accum_ >= samples_per_step_) {
          samples_per_tick_accum_ -= samples_per_step_;
          advanceSequence();
          triggerNote();
        }
      }

      // Smooth frequency transitions
      smooth_hz_ += (current_note_hz_ - smooth_hz_) * 0.01f;
      float w0 = smooth_hz_ * (1.0f / 48000.0f);
      
      // Calculate Oscillator
      float sig = 0.f;
      switch (params_.wave) {
        case SINE:   sig = osc_sinf(phase_); break;
        case SQUARE: sig = osc_sqrf(phase_); break;
        case SAW:
        default:     sig = osc_sawf(phase_); break;
      }
      
      phase_ += w0;
      if (phase_ >= 1.f) phase_ -= 1.f;

      // Envelope logic
      // amp_ follows target_amp_ which is set in triggerNote and modulated by gate length
      // gate check
      float gate_samples = samples_per_step_ * params_.gate;
      if (samples_per_tick_accum_ > gate_samples) {
        target_amp_ = 0.f;
      }

      amp_ += (target_amp_ - amp_) * 0.05f; // click-free smoothing

      float out_val = sig * amp_;
      
      // Mix with dry (as it's a "Generic FX" source)
      // Usually, sound sources in Generic FX might ignore the input or mix it.
      // Spec says: "unit providing a mix between the dry input and the generated arpeggio signal."
      // I'll mix it equally for now or just output the synth. 
      // Most Kaoss sound sources are 100% wet when touched.
      out_p[0] = in_p[0] + out_val;
      out_p[1] = in_p[1] + out_val;
    }
  }

  inline void setParameter(uint8_t index, int32_t value) {
    switch (index) {
      case ROOT:
        params_.root = 36 + (value * (72 - 36) / 1023); // C2 to C5
        updateActiveNotes();
        break;
      case CHORD:
        params_.chord = value;
        updateActiveNotes();
        break;
      case GATE:
        params_.gate = value / 1023.f;
        break;
      case PATTERN:
        params_.pattern = value;
        setTempo(last_tempo_); // refresh timing
        break;
      case MODE:
        params_.mode = value;
        break;
      case RANGE:
        params_.range = std::max((uint8_t)1, (uint8_t)value);
        updateActiveNotes();
        break;
      case WAVE:
        params_.wave = value;
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (index) {
      case ROOT: return (params_.root - 36) * 1023 / (72 - 36);
      case CHORD: return params_.chord;
      case GATE: return (int32_t)(params_.gate * 1023.f);
      case PATTERN: return params_.pattern;
      case MODE: return params_.mode;
      case RANGE: return params_.range;
      case WAVE: return params_.wave;
    }
    return 0;
  }

  inline const char *getParameterStrValue(uint8_t index, int32_t value) const {
    switch (index) {
      case CHORD: {
        static const char *names[] = {"Single", "Major", "Minor", "Dom 7", "Maj 7", "Min 7", "Sus 4", "Dim", "Aug"};
        if (value >= 0 && value < 9) return names[value];
        break;
      }
      case PATTERN: {
        static const char *names[] = {"1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T"};
        if (value >= 0 && value < 8) return names[value];
        break;
      }
      case MODE: {
        static const char *names[] = {"Up", "Down", "Up-Down", "Random", "Seq"};
        if (value >= 0 && value < 5) return names[value];
        break;
      }
      case WAVE: {
        static const char *names[] = {"Saw", "Sine", "Square"};
        if (value >= 0 && value < 3) return names[value];
        break;
      }
    }
    return nullptr;
  }

  inline void touchEvent(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
    (void)id;
    if (phase == k_unit_touch_phase_began) {
      is_touched_ = true;
      samples_per_tick_accum_ = samples_per_step_; // trigger immediately
      seq_index_ = (params_.mode == DOWN) ? (active_notes_count_ - 1) : 0;
      seq_dir_ = 1;
    } else if (phase == k_unit_touch_phase_moved) {
      // Update Root and Chord via X/Y if desired, 
      // but they are already mapped via parameters in header.c
      // So no need to do anything here unless we want to bypass parameter smoothing.
    } else if (phase == k_unit_touch_phase_ended || phase == k_unit_touch_phase_cancelled) {
      is_touched_ = false;
      target_amp_ = 0.f;
    }
  }

private:
  inline void updateActiveNotes() {
    static const int8_t chord_offsets[][4] = {
      {0, 0, 0, 0}, // Single
      {0, 4, 7, 0}, // Major
      {0, 3, 7, 0}, // Minor
      {0, 4, 7, 10},// Dom 7
      {0, 4, 7, 11},// Maj 7
      {0, 3, 7, 10},// Min 7
      {0, 5, 7, 0}, // Sus 4
      {0, 3, 6, 0}, // Dim
      {0, 4, 8, 0}  // Aug
    };
    
    int chord_idx = std::min((uint8_t)params_.chord, (uint8_t)8);
    int notes_in_chord = 0;
    if (chord_idx == 0) notes_in_chord = 1;
    else if (chord_idx == 3 || chord_idx == 4 || chord_idx == 5) notes_in_chord = 4;
    else notes_in_chord = 3;

    active_notes_count_ = 0;
    for (int oct = 0; oct < params_.range; ++oct) {
      for (int i = 0; i < notes_in_chord; ++i) {
        if (active_notes_count_ < 32) {
          active_notes_[active_notes_count_++] = params_.root + chord_offsets[chord_idx][i] + (oct * 12);
        }
      }
    }
  }

  inline void advanceSequence() {
    if (active_notes_count_ == 0) return;

    switch (params_.mode) {
      case UP:
        seq_index_ = (seq_index_ + 1) % active_notes_count_;
        break;
      case DOWN:
        if (seq_index_ == 0) seq_index_ = active_notes_count_ - 1;
        else seq_index_--;
        break;
      case UPDOWN:
        seq_index_ += seq_dir_;
        if (seq_index_ >= (int)active_notes_count_ - 1) {
          seq_index_ = active_notes_count_ - 1;
          seq_dir_ = -1;
        } else if (seq_index_ <= 0) {
          seq_index_ = 0;
          seq_dir_ = 1;
        }
        break;
      case RANDOM:
        seq_index_ = osc_rand() % active_notes_count_;
        break;
      case SEQ:
        // Same as UP for now, or could implement a specific pattern
        seq_index_ = (seq_index_ + 1) % active_notes_count_;
        break;
    }
  }

  inline void triggerNote() {
    if (active_notes_count_ == 0) return;
    uint8_t note = active_notes_[seq_index_ % 32];
    current_note_hz_ = osc_notehzf(note);
    target_amp_ = 1.0f;
    // Potentially reset phase_ = 0 if we want hard retrigger, 
    // but gliding is often nicer. I'll leave it free-running or soft-reset.
  }

  Params params_;
  uint32_t samplerate_{48000};
  uint32_t last_tempo_{120 << 8};

  // Arp State
  uint8_t active_notes_[32];
  uint8_t active_notes_count_;
  int seq_index_;
  int seq_dir_;
  
  float samples_per_step_;
  float samples_per_tick_accum_;
  
  // DSP State
  float phase_;
  float amp_;
  float target_amp_;
  float current_note_hz_;
  float smooth_hz_;

  bool is_active_;
  bool is_touched_;
};
