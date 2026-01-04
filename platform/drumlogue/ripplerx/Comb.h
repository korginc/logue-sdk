// Copyright (C) 2025 tilr
// Comb stereoizer
#pragma once
#include <arm_neon.h>
#include <vector>
class Comb
{
public:
	Comb() {};
	~Comb() {};

  void init(float32_t srate)
  {
    pos = 0;
    buf.resize((int)(20 * srate / 1000), vdup_n_f32(0.0f));
    // std::fill(buf.begin(), buf.end(), 0.0);
    // not needed: https://en.cppreference.com/w/cpp/container/vector/resize
  }

  // comb filter gets in input a sum of stereo samples, stores them in a circular buffer,
  // and outputs a stereoized version of the input by adding and subtracting a delayed version
  // this version uses NEON intrinsics, and uses either a two stereo samples, or
  // four mono one. They will treated in same way, doing a sum and a difference.
//   std::tuple<double, double> process(double input) {
  float32x4_t process(float32x4_t input) {
    buf[pos] = vadd_f32(vget_high_f32(input), vget_low_f32(input));
    pos = (pos + 1) % buf.size();  // circular buffer
    float32x2_t stereoizer;
    vset_lane_f32(0.165, stereoizer, 0); // 0.33 / 2 to equalize the sum of two channels
    vset_lane_f32(-0.165, stereoizer, 1);
    return vaddq_f32(input, vcombine_f32(vmul_lane_f32(stereoizer, buf[pos], 0), vmul_lane_f32(stereoizer, buf[pos], 1)));

    // return std::tuple<double, double>(
      //   input + buf[pos] * 0.33,
      //   input - buf[pos] * 0.33
      // );
  }

private:
  int pos = 0;
  std::vector<float32x2_t> buf;
};