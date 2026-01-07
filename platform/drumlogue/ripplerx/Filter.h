// Copyright 2025 tilr
// Incomplete port of Theo Niessink <theo@taletn.com> rbj filters
#pragma once
#include <arm_neon.h>
#include "float_math.h"

class Filter
{
public:
	Filter() {};
	~Filter() {};

	void lp(float32_t srate, float32_t freq, float32_t q);
	void bp(float32_t srate, float32_t freq, float32_t q);
	void hp(float32_t srate, float32_t freq, float32_t q);
	void clear(float32_t input);
	void reset() { x0 = x1 = x2 = y0 = y1 = y2 = 0.0f; }
	inline float32_t df1(float32_t sample) {
		x2 = x1;
		x1 = x0;
		x0 = sample;

		y2 = y1;
		y1 = y0;
		y0 = b0*x0 + b1*x1 + b2*x2 - a1*y1 - a2*y2;

		return y0;
	}

private:
	float32_t a1 = 0.0;
	float32_t a2 = 0.0;
	float32_t b0 = 0.0;
	float32_t b1 = 0.0;
	float32_t b2 = 0.0;
	float32_t x0 = 0.0;
	float32_t x1 = 0.0;
	float32_t x2 = 0.0;
	float32_t y0 = 0.0;
	float32_t y1 = 0.0;
	float32_t y2 = 0.0;
};