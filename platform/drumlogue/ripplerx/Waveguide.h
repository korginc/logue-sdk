// Copyright 2025 tilr
// Waveguide for OpenTube and ClosedTube models
#pragma once
#include <vector>
#include "float_math.h"

class Waveguide
{
public:
	Waveguide() {
        tube.resize(tube_len);  // Heap allocation
    };
	~Waveguide() {};

	void update(float32_t freq, float32_t vel, bool isRelease);
	float32x2_t process(float32x2_t input);
	void clear();

	bool is_closed = false;
	float32_t srate = 0.0;
	float32_t decay = 0.0;
	float32x2_t radius = vld1_f32(temp);
	float32x2_t max_radius = vld1_f32(temp2);
	float32_t rel = 0.0;
	float32_t vel_decay = 0.0;

private:
	int read_ptr = 0;
	int write_ptr = 0;
	float32_t tube_decay = 0.0;
	std::vector<float32x2_t> tube;
    static const int tube_len = 20000;  // buffer size, 20000 allows for 10Hz at 200k srate (max_size = srate / freq_min)
	// float32x2_t tube[tube_len]; // Heap allocation
	const float32_t temp[2] = {0.0, 0.0};
	const float32_t temp2[2] = {1.0, 1.0};
	// const float32x2_t temp3 = vmov_n_f32(0.0);
	float32x2_t y = vld1_f32(temp);
	float32x2_t y1 = vld1_f32(temp);
};