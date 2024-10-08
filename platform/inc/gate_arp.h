/*
 *  File: gate_arp.h
 *
 *  Gate arpeggiator
 *
 *  2024 (c) Oleg Burdaev
 *  mailto: dukesrg@gmail.com
 */

#pragma once

#include <stdint.h>

#include "patterns.h"

#define MASK_ARP_GATE_ON 1U
#define MASK_ARP_STARTED 2U

struct gate_arp {
  uint32_t sample_counter;
  uint32_t tick_samples;
  uint32_t tick_counter;
  uint32_t state;
  uint32_t unit_num;
  uint32_t unit_samples = 60 * 48000 / 56 / PATTERN_QUANTIZE; //default 120BPM
  uint32_t unit_counter;
  uint32_t pattern_idx;
  const uint8_t *pattern_ptr = patterns;
  void (*gate_on_callback)();
  void (*gate_off_callback)();

  gate_arp() {}

  gate_arp(void (*gate_on)(), void (*gate_off)()) : gate_on_callback{gate_on}, gate_off_callback{gate_off} {}

  inline __attribute__((optimize("Ofast"), always_inline))
  void start() {
    if (state & MASK_ARP_STARTED)
      return; 
    state |= MASK_ARP_GATE_ON | MASK_ARP_STARTED;
    sample_counter = 0;
    pattern_idx = 0;    
    unit_num = pattern_ptr[pattern_idx] >> 4;
    unit_counter = unit_num * unit_samples;
    if (gate_on_callback != nullptr)
      gate_on_callback();
  }

  inline __attribute__((optimize("Ofast"), always_inline))  
  void stop() {
    if (!(state & MASK_ARP_STARTED))
      return; 
    state &= ~(MASK_ARP_GATE_ON | MASK_ARP_STARTED);
    if (gate_off_callback != nullptr)
      gate_off_callback();
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void advance(uint32_t samples) {
    tick_samples += samples;
    if (!(state & MASK_ARP_STARTED))
      return; 
    sample_counter += samples;
    if (sample_counter >= unit_counter) {
      if (state & MASK_ARP_GATE_ON && (unit_num = pattern_ptr[pattern_idx] & 0x0F) != 0) {
        state &= ~MASK_ARP_GATE_ON;
        if (gate_off_callback != nullptr)
          gate_off_callback();
      } else {
        pattern_idx++;
        if (pattern_ptr[pattern_idx] == 0)
          pattern_idx = 0;
        unit_num = pattern_ptr[pattern_idx] >> 4;
        if (!(state & MASK_ARP_GATE_ON)) {
          state |= MASK_ARP_GATE_ON;
          if (gate_on_callback != nullptr)
            gate_on_callback();
        }
      }
      sample_counter -= unit_counter;
      unit_counter = unit_num * unit_samples;
    }
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void set_pattern(uint32_t pattern) {
    static uint32_t current_pattern;
    if (pattern >= PATTERN_COUNT)
      pattern = PATTERN_COUNT - 1;
    if (pattern != current_pattern) {
      if (pattern == 0) {
        current_pattern = 0;
        pattern_ptr = patterns;
      } else {
        for (; pattern > current_pattern; current_pattern++)
          while (*(pattern_ptr++));
        for (; pattern < current_pattern; current_pattern--)
          while (*(--pattern_ptr - 1));
      }
      pattern_idx = 0;
    }
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  void set_tempo(uint32_t tempo) {
    unit_samples = ((uint32_t)60 * 48000 / PATTERN_QUANTIZE << 14) / tempo << 2;
    unit_counter = unit_num * unit_samples;
  }
  inline __attribute__((optimize("Ofast"), always_inline))

  void set_tempo_4ppqn_tick(uint32_t counter) {
    if (tick_counter != 0) {
      unit_samples = tick_samples * 4 / PATTERN_QUANTIZE;
      unit_counter = unit_num * unit_samples;
    }
    tick_samples = 0;
    tick_counter = counter;
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  bool is_gate_on() {
    return state & MASK_ARP_GATE_ON;
  }

  inline __attribute__((optimize("Ofast"), always_inline))
  float out() {
    return state & MASK_ARP_GATE_ON ? 1.f : 0.f;
  }
};

#undef MASK_ARP_GATE_ON
#undef MASK_ARP_STARTED
