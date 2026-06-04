#pragma once
#include <algorithm>
#include <array>
#include <cstdlib>

/**
 * @brief round-robin, closest note voice stealing voice allocator
 * @author Shijie Xia, Korg Inc.
 * @note simplfied version of H. Fujii's assigner
 */

#define INVALID_VOICE 0xFF

template <size_t N>
class VoiceAllocator {
public:
  VoiceAllocator() { voices.fill(-1); }

  void set_roundrobin(bool enabled) { roundrobin = enabled; }

  uint8_t note_on(uint8_t note_number) {
    if (!roundrobin)
      next_voice = 0;

    // 1) retrigger if already playing
    auto it = std::find(voices.begin(), voices.end(), note_number);
    if (it != voices.end()) {
      return static_cast<uint8_t>(std::distance(voices.begin(), it));
    }

    // 2) claim a free voice (round-robin)
    for (uint8_t k = 0; k < N; ++k) {
      uint8_t slot = (next_voice + k) % N;
      if (voices[slot] < 0) {
        voices[slot] = note_number;
        next_voice = (slot + 1) % N;
        return slot;
      }
    }

    // 3) steal the closest note
    uint8_t victim = next_voice;
    for (uint8_t k = 1; k < N; ++k) {
      uint8_t slot = (next_voice + k) % N;
      if (std::abs(voices[slot] - note_number) <
          std::abs(voices[victim] - note_number)) {
        victim = slot;
      }
    }

    voices[victim] = note_number;
    next_voice = (victim + 1) % N;
    return victim;
  }

  uint8_t note_off(uint8_t note_number) {
    auto it = std::find(voices.begin(), voices.end(), note_number);
    if (it == voices.end())
      return INVALID_VOICE;
    *it = -1;
    return static_cast<uint8_t>(std::distance(voices.begin(), it));
  }

private:
  bool roundrobin = true;
  uint8_t next_voice = 0;
  std::array<int8_t, N> voices;
};
