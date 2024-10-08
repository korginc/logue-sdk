/*
 *  File: envelopes.h
 *
 *  Envelope generators
 *
 *  2024 (c) Oleg Burdaev
 *  mailto: dukesrg@gmail.com
 */

#pragma once

#include <stdint.h>

#include "float_math.h"

#define STAGE_IDLE 0U
#define STAGE_ATTACK 1U
#define STAGE_DECAY 2U
#define STAGE_SUSTAIN 3U
#define STAGE_RELEASE 4U

struct eg_adsr {
  uint32_t stage;
  float attack_min_level = 0.f;
  float attack_max_level = 1.f;
  float idle_treshold = -144.4944f; //24-bit dBFS is small enough to finish with decaying
  float idle_level = 0.f;
  float decay_rate_ref_level = -60.f;
  float level;
  float rate;
  float attack_rate;
  float decay_rate;
  float sustain_level;
  float release_rate;

  inline __attribute__((optimize("Ofast"), always_inline))
  void set_stage_idle() {
    level = idle_level;
    stage = STAGE_IDLE;
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void set_stage_release() {
    if (release_rate == 0.f) {
      set_stage_idle();
    } else {
      rate = release_rate;
      if (stage != STAGE_DECAY)
        level = fasterampdbf(level);
      stage = STAGE_RELEASE;
    }
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void set_stage_sustain() {
    level = fasterdbampf(sustain_level);
    stage = STAGE_SUSTAIN;
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void set_stage_decay() {
    if (decay_rate == 0.f) {
      set_stage_sustain();
    } else {
      rate = decay_rate;
      level = fasterampdbf(attack_max_level);
      stage = STAGE_DECAY;
    }
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void set_stage_attack() {
    if (attack_rate == 0.f) {
      set_stage_decay();
    } else {
      if (stage == STAGE_DECAY || stage == STAGE_RELEASE)
        level = fasterdbampf(level);
      if (attack_min_level > level)
        level = attack_min_level;
      stage = STAGE_ATTACK;
    }
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void start() {
    set_stage_attack();
  }

  inline __attribute__((optimize("Ofast"), always_inline))  
  void stop() {
    if (stage != STAGE_IDLE)
      set_stage_release();
  }

  inline __attribute__((optimize("Ofast"), always_inline))  
  void set_attack(float attack_time_s) {
    if (attack_time_s == 0.f) {
      attack_rate = 0.f;
      if (stage == STAGE_ATTACK)
        set_stage_decay();
    } else {
      attack_rate = .208333333e-4f * (attack_max_level - idle_level) / attack_time_s; //1/48000
    }
  }
  
  inline __attribute__((optimize("Ofast"), always_inline))  
  void set_decay(float decay_time_s) {
    if (decay_time_s == 0.f) {
      decay_rate = 0.f;
      if (stage == STAGE_DECAY)
        set_stage_sustain();
    } else {
      decay_rate = .208333333e-4f * decay_rate_ref_level / decay_time_s; //1/48000
    }
    if (stage == STAGE_DECAY)
      rate = decay_rate;
  }

  inline __attribute__((optimize("Ofast"), always_inline))  
  void set_sustain(float sustain_level_db) {
    sustain_level = sustain_level_db;
    if (stage == STAGE_SUSTAIN)
      level = fasterdbampf(sustain_level);
  }

  inline __attribute__((optimize("Ofast"), always_inline))  
  void set_release(float release_time_s) {
    if (release_time_s == 0.f) {
      release_rate = 0.f;
      if (stage == STAGE_RELEASE)
        set_stage_idle();
    } else {
      release_rate = .208333333e-4f * decay_rate_ref_level / release_time_s; //1/48000
    }
    if (stage == STAGE_RELEASE)
      rate = release_rate;      
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  float out() {
    return stage == STAGE_DECAY || stage == STAGE_RELEASE ? fasterdbampf(level) : level;
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void advance(uint32_t samples) {
    if (stage == STAGE_ATTACK) {
      level += attack_rate * samples;
      if (level < attack_max_level)
        return;
      samples = attack_rate == 0.f ? 0 : (level - attack_max_level) / attack_rate;
      set_stage_decay();
    }
    if (stage == STAGE_DECAY || stage == STAGE_RELEASE) {
      level += rate * samples;
      if (level < idle_treshold) {
        set_stage_idle();
      } else if (stage == STAGE_DECAY && level < sustain_level) {
        set_stage_sustain();
      }
    }
  }
};

#undef STAGE_IDLE
#undef STAGE_ATTACK
#undef STAGE_DECAY
#undef STAGE_SUSTAIN
#undef STAGE_RELEASE
