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
    buf_size = (int)(20 * srate / 1000);
    buf.resize(buf_size, vdup_n_f32(0.0f));

    // Precompute stereoizer constant: [+0.165, -0.165]
    float32x2_t tmp = vdup_n_f32(0.165f);
    stereoizer = vset_lane_f32(-0.165f, tmp, 1);
  }

  // comb filter gets in input a sum of stereo samples, stores them in a circular buffer,
  // and outputs a stereoized version of the input by adding and subtracting a delayed version
  // this version uses NEON intrinsics, and uses either a two stereo samples, or
  // four mono one. They will treated in same way, doing a sum and a difference.
  float32x4_t process(float32x4_t input) {
    // Sum the high and low stereo pairs
    float32x2_t input_sum = vadd_f32(vget_high_f32(input), vget_low_f32(input));

    // Store sum in circular buffer
    buf[pos] = input_sum;

    // Read delayed value BEFORE incrementing position
    float32x2_t delayed = buf[pos];

    // Increment to next position
    pos = (pos + 1) % buf_size;

    // Apply stereoizer effect: [delayed * 0.165, delayed * -0.165]
    float32x2_t stereo_effect = vmul_f32(delayed, stereoizer);

    // Broadcast stereo effect to both channels and add to input
    return vaddq_f32(input, vcombine_f32(stereo_effect, stereo_effect));
  }

private:
  int pos = 0;
  int buf_size = 0;
  float32x2_t stereoizer;
  std::vector<float32x2_t> buf;
};