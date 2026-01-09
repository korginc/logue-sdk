#include "Filter.h"

void Filter::lp(float32_t srate, float32_t freq, float32_t q)
{
	auto w0 = M_2_PI * fmin(freq / srate, 0.49);
	auto alpha = fastersinfullf(w0) / (2.0 * q);

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
	auto alpha = fastersinfullf(w0) / (2.0 * q);

	auto a0 = 1.0 + alpha;
	auto scale = 1.0 / a0;
	a1 = fastcosfullf(w0) * -2.0 * scale;
	a2 = (1.0 - alpha) * scale;

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
	x0 = x1 = x2 = input;
	y0 = y1 = y2 = input * (b0 + b1 + b2) / (1.0 + a1 + a2);
}