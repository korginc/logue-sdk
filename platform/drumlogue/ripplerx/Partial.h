// Copyright 2025 tilr
// Partial is a second order bandpass filter with extra variables for decay, frequency and amplitude calculation

#pragma once
#include "float_math.h"
#include "constants.h"

class Partial
{
public:
	Partial(int n) { k = n; };
	~Partial() {};

	void update(float32_t freq, float32_t ratio, float32_t ratio_max, float32_t vel, bool isRelease);
	float32x2_t process(float32x2_t input);
	void clear();

	float32_t srate = 0.0;
	int k = 0; // Partial num
	float32_t decay = 0.0;
	float32_t damp = 0.0;
	float32_t tone = 0.0;
	float32_t hit = 0.0;
	float32_t rel = 0.0;
	float32_t inharm = 0.0;
	float32_t radius = 0.0;
	float32_t vel_decay = 0.0;
	float32_t vel_hit = 0.0;
	float32_t vel_inharm = 0.0;

private:
	float32_t b0 = 0.0;
	float32_t b2 = 0.0;
	float32_t a0 = 1.0;
	float32_t a1 = 0.0;
	float32_t a2 = 0.0;

	float32x2_t x1 = vmov_n_f32(0.0);
	float32x2_t x2 = vmov_n_f32(0.0);
	float32x2_t y1 = vmov_n_f32(0.0);
	float32x2_t y2 = vmov_n_f32(0.0);
};