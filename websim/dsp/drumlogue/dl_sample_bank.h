// websim host stand-in for the drumlogue on-board PCM sample banks.
//
// On hardware the runtime descriptor exposes get_num_sample_banks /
// get_num_samples_for_bank / get_sample, handing units the device's factory and
// user samples as sample_wrapper_t (interleaved float PCM). The browser has no
// such sample memory, so sample-playback units (e.g. SmplVox / sample-voice)
// would see null accessors and refuse to initialise.
//
// This provides a small synthetic bank so those units run and make sound in the
// sim. The content is generated procedurally (no file decoding / FS needed),
// which keeps it deterministic for the offline render checks. See
// WEBSIM_FOLLOWUP_PLAN.md §C.4.
#ifndef WEBSIM_DL_SAMPLE_BANK_H_
#define WEBSIM_DL_SAMPLE_BANK_H_

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "sample_wrapper.h"

namespace websim {

struct DlSampleBank {
  std::vector<std::vector<float>> data;     // owning sample buffers (mono)
  std::vector<sample_wrapper_t> wrappers;   // metadata pointing into `data`
};

inline DlSampleBank &dl_sample_bank() {
  static DlSampleBank b;
  if (!b.wrappers.empty())
    return b;

  constexpr int kSr = 48000;
  constexpr int kLen = kSr / 2;  // 0.5 s mono one-shots
  b.data.reserve(8);             // reserve so sample_ptr stays valid on growth
  b.wrappers.reserve(8);

  auto add = [&](const char *name, std::vector<float> samp) {
    b.data.push_back(std::move(samp));
    sample_wrapper_t w = {};
    w.bank = 0;
    w.index = (uint8_t)b.wrappers.size();
    w.channels = 1;
    std::strncpy(w.name, name, UNIT_SAMPLE_WRAPPER_MAX_NAME_LEN);
    w.name[UNIT_SAMPLE_WRAPPER_MAX_NAME_LEN] = '\0';
    w.frames = b.data.back().size();
    w.sample_ptr = b.data.back().data();
    b.wrappers.push_back(w);
  };

  const double kPi = 3.14159265358979323846;

  // 0: pure 220 Hz sine tone.
  {
    std::vector<float> s(kLen);
    for (int i = 0; i < kLen; ++i)
      s[i] = 0.8f * (float)std::sin(2.0 * kPi * 220.0 * i / kSr);
    add("Sine 220", std::move(s));
  }
  // 1: white noise (LCG) — a snare/hat stand-in once the voice's decay shapes it.
  {
    std::vector<float> s(kLen);
    uint32_t r = 0x12345678u;
    for (int i = 0; i < kLen; ++i) {
      r = r * 1664525u + 1013904223u;
      s[i] = 0.6f * ((float)(r >> 8) * (1.0f / 8388608.0f) - 1.0f);
    }
    add("Noise", std::move(s));
  }
  // 2: A-major triad (220/277.18/329.63) — a tonal/chord stand-in.
  {
    std::vector<float> s(kLen);
    const double f[3] = {220.0, 277.18, 329.63};
    for (int i = 0; i < kLen; ++i) {
      double acc = 0.0;
      for (double fk : f)
        acc += std::sin(2.0 * kPi * fk * i / kSr);
      s[i] = (float)(0.3 * acc);
    }
    add("Chord A", std::move(s));
  }

  return b;
}

inline uint8_t websim_get_num_sample_banks() { return 1; }

inline uint8_t websim_get_num_samples_for_bank(uint8_t bank) {
  return bank == 0 ? (uint8_t)dl_sample_bank().wrappers.size() : 0;
}

inline const sample_wrapper_t *websim_get_sample(uint8_t bank, uint8_t idx) {
  if (bank != 0)
    return nullptr;
  auto &w = dl_sample_bank().wrappers;
  if (idx >= w.size())
    return nullptr;
  return &w[idx];
}

}  // namespace websim

#endif  // WEBSIM_DL_SAMPLE_BANK_H_
