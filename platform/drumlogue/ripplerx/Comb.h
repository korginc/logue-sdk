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
    buf.resize((int)(20 * srate / 1000), 0);
    // this is not needed per:
    // https://en.cppreference.com/w/cpp/container/vector/resize
    // std::fill(buf.begin(), buf.end(), 0.0);
  }

//   std::tuple<double, double> process(double input) {
  float32x2_t process(float32x2_t input) {
    buf[pos] = input;
    pos = (pos + 1) % buf.size();  // circular buffer
    float32x2_t stereoizer;
    vset_lane_f32(0.33, stereoizer, 0); // Set lanes within a vector
    vset_lane_f32(-0.33, stereoizer, 1);
    return vfma_f32(input, buf[pos], stereoizer);

    // return std::tuple<double, double>(
    //   input + buf[pos] * 0.33,
    //   input - buf[pos] * 0.33
    // );
  }

private:
  int pos = 0;
  std::vector<float32x2_t> buf;
};