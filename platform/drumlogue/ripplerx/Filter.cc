#include "Filter.h"

void Filter::lp(float32_t srate, float32_t freq, float32_t q)
{
	auto w0 = M_2_PI * fmin(freq / srate, 0.49f);
	auto alpha = fastersinfullf(w0) / (2.0f * q);

	auto a0 = 1.0f + alpha;
	auto scale = 1.0f / a0;
	a1 = fastcosfullf(w0) * -2.0f * scale;
	a2 = (1.0f - alpha) * scale;

	b2 = b0 = (1.0f + a1 + a2) * 0.25f;
	b1 = b0 * 2.0f;
}

void Filter::bp(float32_t srate, float32_t freq, float32_t q)
{
	auto w0 = M_2_PI * fmin(freq / srate, 0.49f);
	auto alpha = fastersinfullf(w0) / (2.0f * q);

	auto a0 = 1.0f + alpha;
	auto scale = 1.0f / a0;
	a1 = fastcosfullf(w0) * -2.0f * scale;
	a2 = (1.0f - alpha) * scale;

	b2 = -(b0 = (1.0f - a2) * 0.5f * q);
	b1 = 0.0f;
}

void Filter::hp(float32_t srate, float32_t freq, float32_t q)
{
	auto w0 = M_2_PI * fmin(freq / srate, 0.49f);
	auto alpha = fastersinfullf(w0) / (2.0f * q);

	auto a0 = 1.0f + alpha;
	auto scale = 1.0f / a0;
	a1 = fastcosfullf(w0) * -2.0f * scale;
	a2 = (1.0f - alpha) * scale;

	b2 = b0 = (1.0f - a1 + a2) * 0.25f;
	b1 = b0 * -2.0f;
}

void Filter::clear(float32_t input)
{
	x0 = x1 = x2 = input;
	float32_t denom = 1.0f + a1 + a2;
	y0 = y1 = y2 = (si_fabsf(denom) > 1e-10f) ? input * (b0 + b1 + b2) / denom : 0.0f;
}
