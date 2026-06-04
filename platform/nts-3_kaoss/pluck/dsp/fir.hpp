#pragma once
#include <cmath>

#include "utility.hpp"

// 3-tap symmetric FIR: coefficients [d, 1-2d, d].
// d in [0, 0.25], larger d = more lowpass filtering
// linear phase, group delay is always 1 sample regardless of d
class SymmetricFir3
{
public:
  //
  void set_damp(float damp)
  {
    // [0, 1] -> [0, 0.25]
    const float a = clampf(damp / 4.f, 0.f, 0.25f);
    h1 = 1 - 2.f * a;
    h0 = a;
  }

  // Magnitude of the FIR at normalized frequency w (radians/sample)
  float magnitude_at(float w) const
  {
    return h1 + 2.f * h0 * std::cos(w);
  }

  void reset()
  {
    z1 = 0.f;
    z2 = 0.f;
  }

  float process_sample(float x)
  {
    float y = h1 * z1 + h0 * (x + z2);
    z2 = z1;
    z1 = x;
    return y;
  }

private:
  float h0 = 0.f, h1 = 1.f;
  float z1 = 0.f, z2 = 0.f;
};
