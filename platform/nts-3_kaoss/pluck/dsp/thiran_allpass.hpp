#pragma once
#include <array>

// Cascade of M identical first-order Thiran allpass filters.
// Transfer function per stage: H(z) = (a1 + z^-1) / (1 + a1*z^-1)
// Difference equation:          y[n] = a1*(x[n] - y[n-1]) + x[n-1]
//
// a1 = (1 - D) / (D + 1)  where D is the desired DC group delay per stage.
// With all stages sharing the same a1, the cascade is computationally cheap:
// M × (2 multiplies + 1 add) per sample.
template <int M>
class ThiranAllpassCascade
{
  static_assert(M >= 1, "M must be at least 1");

public:
  void set_coeff(float a) { a1 = a; }

  float process_sample(float x)
  {
    for (int i = 0; i < M; ++i)
    {
      float y = a1 * (x - yp[i]) + xp[i];
      xp[i] = x;
      yp[i] = y;
      x = y;
    }
    return x;
  }

  void reset()
  {
    xp.fill(0.f);
    yp.fill(0.f);
  }

private:
  float a1 = 0.f;
  std::array<float, M> xp = {};
  std::array<float, M> yp = {};
};
