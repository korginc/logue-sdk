#pragma once
#include <cmath>

// first-order IIR filter
// H(z) = (a0 + a1*z^-1) / (1 + b1*z^-1)
class OnePole
{
public:
  void set_lp(float cutoff_norm)
  {
    float c = std::exp(-two_pi * cutoff_norm);
    set_coeffs(1.f - c, 0.f, -c);
  }

  void set_hp(float cutoff_norm)
  {
    float c = std::exp(-two_pi * cutoff_norm);
    set_coeffs(0.5f * (1.f + c), -0.5f * (1.f + c), -c);
  }

  void set_coeffs(float a0, float a1, float b1)
  {
    this->a0 = a0;
    this->a1 = a1;
    this->b1 = b1;
  }

  void reset() { s1 = 0.f; }

  float process_sample(float x)
  {
    float y = a0 * x + s1;
    s1 = a1 * x - b1 * y;
    return y;
  }

private:
  static constexpr float two_pi = 2.f * M_PI;

  float a0 = 0.5f, a1 = 0.5f, b1 = 0.f;
  float s1 = 0.f;
};
