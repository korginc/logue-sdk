#pragma once
#include <cmath>
#include <algorithm>

constexpr float A440Frequency = 440.0f;
constexpr int A440NoteNumber = 69;

inline float note_to_hz(int note)
{
  return A440Frequency *
         std::pow(2.0f, (static_cast<float>(note) - A440NoteNumber) / 12.0f);
}

// [0,1] -> [20, 20000] Hz
inline float norm_to_freq(float x)
{
  return 2.f * powf(10.f, 3.f * x + 1.f); 
}

inline float lerpf(float x0, float x1, float frac)
{
  return x0 + frac * (x1 - x0);
}

inline float clampf(float x, float lo, float hi)
{
  return std::min(std::max(x, lo), hi);
}

// https://www.desmos.com/calculator/merzdbysff
inline float fast_tanh(float x)
{
  return x * (27.f + x * x) / (27.f + 9.f * x * x);
}