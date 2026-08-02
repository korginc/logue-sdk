// Minimal WAV writer for the websim offline render harness.
//
// Writes 32-bit IEEE-float PCM (WAVE format tag 3), which is lossless and read
// directly by scipy.io.wavfile / numpy for the golden spot-checks (see
// WEBSIM_FOLLOWUP_PLAN.md §B). Header-only; used by the render bridges.
#ifndef WEBSIM_WAV_WRITER_H_
#define WEBSIM_WAV_WRITER_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace websim {

// Write interleaved float samples (range nominally [-1,1], not clamped) as a
// 32-bit float WAV. Returns true on success.
inline bool write_wav_f32(const char *path, const std::vector<float> &interleaved,
                          int channels, int sample_rate) {
  FILE *f = std::fopen(path, "wb");
  if (!f) return false;

  const uint32_t data_bytes = (uint32_t)(interleaved.size() * sizeof(float));
  const uint16_t fmt_tag = 3;        // IEEE float
  const uint16_t bits = 32;
  const uint16_t ch = (uint16_t)channels;
  const uint32_t byte_rate = (uint32_t)sample_rate * ch * (bits / 8);
  const uint16_t block_align = (uint16_t)(ch * (bits / 8));
  const uint32_t fmt_chunk = 16;
  const uint32_t riff_size = 4 + (8 + fmt_chunk) + (8 + data_bytes);

  auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
  auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };

  std::fwrite("RIFF", 1, 4, f);
  w32(riff_size);
  std::fwrite("WAVE", 1, 4, f);
  std::fwrite("fmt ", 1, 4, f);
  w32(fmt_chunk);
  w16(fmt_tag);
  w16(ch);
  w32((uint32_t)sample_rate);
  w32(byte_rate);
  w16(block_align);
  w16(bits);
  std::fwrite("data", 1, 4, f);
  w32(data_bytes);
  std::fwrite(interleaved.data(), sizeof(float), interleaved.size(), f);

  const bool ok = (std::ferror(f) == 0);
  std::fclose(f);
  return ok;
}

}  // namespace websim

#endif  // WEBSIM_WAV_WRITER_H_
