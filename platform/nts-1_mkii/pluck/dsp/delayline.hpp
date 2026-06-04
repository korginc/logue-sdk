#pragma once
#include <cstddef>
#include <array>

#include "utility.hpp"

template <size_t N>
class DelayLine
{
  static_assert((N > 1) && ((N & (N - 1)) == 0), "N must be power of 2");

public:
  void write(float sample)
  {
    ++current_pos;
    samples[current_pos & mask] = sample;
  }

  void clear()
  {
    samples.fill(0.f);
  }

  float read_linear(float delay) const
  {
    size_t d = static_cast<size_t>(delay);
    float frac = delay - static_cast<float>(d);
    return lerpf(tap(d), tap(d + 1), frac);
  }

  float read_lagrange_2nd(float delay) const
  {
    size_t d = static_cast<size_t>(delay);
    float frac = delay - static_cast<float>(d);
    float x0 = tap(d), x1 = tap(d + 1), x2 = tap(d + 2);
    float c0 = (frac - 1.f) * (frac - 2.f) * 0.5f;
    float c1 = -frac * (frac - 2.f);
    float c2 = frac * (frac - 1.f) * 0.5f;
    return x0 * c0 + x1 * c1 + x2 * c2;
  }

  float read_lagrange_3rd(float delay) const
  {
    size_t d = static_cast<size_t>(delay);
    float frac = delay - static_cast<float>(d);
    float x0 = tap(d), x1 = tap(d + 1), x2 = tap(d + 2), x3 = tap(d + 3);
    float d1 = frac - 1.f, d2 = frac - 2.f, d3 = frac - 3.f;
    float c0 = -d1 * d2 * d3 / 6.f;
    float c1 = frac * d2 * d3 * 0.5f;
    float c2 = -frac * d1 * d3 * 0.5f;
    float c3 = frac * d1 * d2 / 6.f;
    return x0 * c0 + x1 * c1 + x2 * c2 + x3 * c3;
  }

private:
  float tap(size_t delay) const
  {
    return samples[(current_pos - delay) & mask];
    // if delay > current_pos, underflow happens but (& mask) makes it semantically correct
  }

  static constexpr size_t mask = N - 1;
  std::array<float, N> samples = {};
  size_t current_pos = 0;
};
