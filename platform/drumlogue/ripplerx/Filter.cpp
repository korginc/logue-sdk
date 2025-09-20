#include "Filter.h"

void Filter::lp(float32_t srate, float32_t freq, float32_t q)
{
	auto w0 = M_2_PI * fmin(freq / srate, 0.49);
	auto alpha = fastersinfullf(w0) / 2.0*q;

	auto a0 = 1.0 + alpha;
	auto scale = 1.0 / a0;
	a1 = fastcosfullf(w0) * -2.0 * scale;
	a2 = (1.0 - alpha) * scale;

	b2 = b0 = (1.0 + a1 + a2) * 0.25;
	b1 = b0 * 2.0;
}

void Filter::bp(float32_t srate, float32_t freq, float32_t q)
{
	auto w0 = M_2_PI * fmin(freq / srate, 0.49);
	auto alpha = fastersinfullf(w0) / 2.0*q;

	auto a0 = 1.0 + alpha;
	auto scale = 1.0 / a0;
	a1 = fastcosfullf(w0) * -2.0 * scale;
	a2 = (1 - alpha) * scale;

	b2 = -(b0 = (1 - a2) * 0.5 * q);
	b1 = 0.0;
}

void Filter::hp(float32_t srate, float32_t freq, float32_t q)
{
	auto w0 = M_2_PI * fmin(freq / srate, 0.49);
	auto alpha = fastersinfullf(w0) / (2.0 * q);

	auto a0 = 1.0 + alpha;
	auto scale = 1.0 / a0;
	a1 = fastcosfullf(w0) * -2.0 * scale;
	a2 = (1.0 - alpha) * scale;

	b2 = b0 = (1.0 - a1  + a2) * 0.25;
	b1 = b0 * -2.0;
}

void Filter::clear(float32_t input)
{
	x0 = x1 = input;
	y0 = y1 = input / (1.0 + a1 + a2) * (b0 + b1 + b2);
}

float32_t Filter::df1(float32_t sample)
{
	auto x2 = x1;
	x1 = x0;
	x0 = sample;

	auto y2 = y1;
	y1 = y0;
	y0 = b0*x0 + b1*x1 + b2*x2 - a1*y1 - a2*y2;

	return y0;
}