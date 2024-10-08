/*
 *  File: peak_detect.h
 *
 *  Peak detector
 *
 *  2024 (c) Oleg Burdaev
 *  mailto: dukesrg@gmail.com
 */

#pragma once

#include <stdint.h>

#include "float_math.h"

#define PEAK_LOW 0U
#define PEAK_HIGH 1U

struct peak_detect {
  uint32_t state;
  uint32_t sample_counter;
  uint32_t time_counter;
  float threshold_level;
  void (*peak_high_callback)();
  void (*peak_low_callback)();

  peak_detect() {}

  peak_detect(void (*peak_high)(), void (*peak_low)()) : peak_high_callback{peak_high}, peak_low_callback{peak_low} {}

  inline __attribute__((optimize("Ofast"), always_inline))  
  void set_threshold(float threshold_level_db) {
    threshold_level = fasterdbampf(threshold_level_db);
  }

  inline __attribute__((optimize("Ofast"), always_inline))  
  void set_time(float time_s) {
    time_counter = 48000 * time_s;
  }

  inline __attribute__((optimize("Ofast"), always_inline))  
  void process(const float * in, uint32_t frames, uint32_t channels, uint32_t channel_mask) {
    const float * __restrict in_p = in;
    const float * in_e = in_p + frames * channels;
    bool high_pending = false;
    for (; in_p != in_e; in_p += channels) {
      sample_counter++;
      for (uint32_t i = 1 << channels; i >>= 1; i >>= 1) {
        if (i & channel_mask && abs(*in_p) > threshold_level) {
          high_pending = true;
          sample_counter = 0;
          break;
        }
      }
    }
    if (time_counter != 0 && sample_counter > time_counter) {
      if (state == PEAK_HIGH) {
        state = PEAK_LOW;
        if (peak_low_callback != nullptr)
          peak_low_callback();
      }
    } else if (high_pending) {
      if (state == PEAK_LOW) {
        state = PEAK_HIGH;
        if (peak_high_callback != nullptr)
          peak_high_callback();
      }
    }
  }  
  
  inline __attribute__((optimize("Ofast"), always_inline))
  bool is_high() {
    return state == PEAK_HIGH;
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  float out() {
    return state == PEAK_HIGH ? 1.f : 0.f;
  }
};

#undef PEAK_LOW
#undef PEAK_HIGH
