#include "Partial.h"

void Partial::update(float32_t f_0, float32_t ratio, float32_t ratio_max, float32_t vel, bool isRelease)
{
    // auto inharm_k = fmin(1.0, exp(log(inharm) + vel * vel_inharm * -log(0.0001))) - 0.0001; // normalize velocity contribution on a logarithmic scale
	auto log_vel = vel * M_TWOLN100;
	auto inharm_k = fmin(1.0, e_expf(fasterlogf(inharm) + log_vel * vel_inharm)) - 0.0001; // normalize velocity contribution on a fasterlogfarithmic scale
    inharm_k = fasterSqrt(1 + inharm_k * fasterpow2f(ratio - 1));
	auto f_k = f_0 * ratio * inharm_k;

    // auto decay_k = fmin(100.0, exp(log(decay) + vel * vel_decay * (log(100.0) - log(0.01)))); // normalize velocity contribution on a logarithmic scale
	auto decay_k = fmin(100.0, e_expff(fasterlogf(decay) + log_vel * vel_decay)); // normalize velocity contribution on a fasterlogfarithmic scale
	if (isRelease)
		decay_k *= rel;
    // clear
	if (f_k >= (0.48 * srate) || f_k < 20.0 || decay_k == 0.0) {
		b0 = b2 = a1 = a2 = 0.0;
		a0 = 1.0;
		return;
	}

	auto f_max = fmin(20000.0, f_0 * ratio_max * inharm_k);
	auto omega = (M_TWOPI * f_k) / srate;
	auto alpha = M_TWOPI / srate; // aprox 1 sec decay

	// auto damp_k = damp <= 0
	// 	? fasterpowf(f_0 / f_k, (damp * 2.0))
	// 	: fasterpowf(f_max / f_k, (damp * 2.0));
	auto damp_k =  fasterpowf((damp <= 0 ? f_0 : f_max) /
                                         f_k, (damp * 2.0));

	decay_k /= damp_k;

	auto tone_gain = tone <= 0
		// ? fasterpowf(f_k / f_0, tone * 12 / 6)
		// : fasterpowf(f_k / f_max, tone * 12 / 6);
		? fasterpowf(f_k / f_0, tone * 2)
		: fasterpowf(f_k / f_max, tone * 2);
    //auto amp_k = fabs(sin(juce::MathConstants<double>::pi * k * fmin(.5, hit + vel_hit * vel / 2.0)));
	auto amp_k = 35.0 * si_fabsf(fastersinfullf(M_PI * k * fmin(.5, hit + vel_hit * vel / 2.0)));

	// Bandpass filter coefficients (normalized)
	b0 = alpha * tone_gain * amp_k;
	// b2 = -alpha * tone_gain * amp_k;
	b2 = -b0;
	a0 = decay_k ? 1.0 + alpha / decay_k : 0.0;
	a1 = -2.0 * fastercosfullf(omega);
	a2 = decay_k ? 1.0 - alpha / decay_k : 0.0;
}

float32x2_t Partial::process(float32x2_t input)
{
	// auto output = ((b0 * input + b2 * x2) - (a1 * y1 + a2 * y2)) / a0;
    float32x2_t temp = vmov_n_f32(a0);  // use multiply by reciprocal for division
	// vmla_f32 = RESULT[I] = a[i] + (b[i] * c[i]) for i = 0 to 1
    auto output = vmul_f32(vsub_f32( vmla_f32(vmul_n_f32(x2, b2), input, b0),
    vmla_f32(vmul_n_f32(y2, a2), y1, a1)), vrecpe_f32(temp));
	x2 = x1;
	x1 = input;
	y2 = y1;
	y1 = output;

	return output;
}

void Partial::clear()
{
	x1 = x2 = y1 = y2 = vmov_n_f32(0.0);
}