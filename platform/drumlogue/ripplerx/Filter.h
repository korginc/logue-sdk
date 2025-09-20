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
	float32_t df1(float32_t sample);

private:
	float32_t a1 = 0.0;
	float32_t a2 = 0.0;
	float32_t b0 = 0.0;
	float32_t b1 = 0.0;
	float32_t b2 = 0.0;
	float32_t x0 = 0.0;
	float32_t x1 = 0.0;
	float32_t y0 = 0.0;
	float32_t y1 = 0.0;
};