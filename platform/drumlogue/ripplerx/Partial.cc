#include "Partial.h"
#include "constants.h"

void Partial::update(float32_t f_0, float32_t ratio, float32_t ratio_max, float32_t vel, bool isRelease)
{
    // auto inharm_k = fmin(1.0f, exp(log(inharm) + vel * vel_inharm * -log(0.0001f))) - 0.0001f; // normalize velocity contribution on a logarithmic scale
	auto log_vel = vel * M_TWOLN100;
	auto inharm_k = fmin(1.0f, e_expf(fasterlogf(inharm) + log_vel * vel_inharm)) - 0.0001f; // normalize velocity contribution on a logarithmic scale
    inharm_k = fasterSqrt(1.0f + inharm_k * fasterpow2f(ratio - 1.0f));
	auto f_k = f_0 * ratio * inharm_k;

    // auto decay_k = fmin(100.0f, exp(log(decay) + vel * vel_decay * (log(100.0f) - log(0.01f)))); // normalize velocity contribution on a logarithmic scale
	auto decay_k = fmin(100.0f, e_expff(fasterlogf(decay) + log_vel * vel_decay)); // normalize velocity contribution on a logarithmic scale
	if (isRelease)
		decay_k *= rel;
    // clear if out of range
	const float32_t f_nyquist = c_nyquist_factor * srate;
	if (f_k >= f_nyquist || f_k < c_freq_min || decay_k < c_decay_min) {
		b0 = b2 = a1 = a2 = 0.0f;
		a0 = 1.0f;
		return;
	}

	auto f_max = fmin(c_freq_max, f_0 * ratio_max * inharm_k);
	auto omega = (M_TWOPI * f_k) / srate;
	auto alpha = M_TWOPI / srate; // aprox 1 sec decay

	// auto damp_k = damp <= 0
	// 	? fasterpowf(f_0 / f_k, (damp * 2.0f))
	// 	: fasterpowf(f_max / f_k, (damp * 2.0f));
	auto damp_k =  fasterpowf((damp <= 0 ? f_0 : f_max) /
                                         f_k, (damp * 2.0f));

	decay_k /= damp_k;

	auto tone_gain = tone <= 0
		// ? fasterpowf(f_k / f_0, tone * 12 / 6)
		// : fasterpowf(f_k / f_max, tone * 12 / 6);
		? fasterpowf(f_k / f_0, tone * 2.0f)
		: fasterpowf(f_k / f_max, tone * 2.0f);
    // Velocity modulates hit envelope shaping
	auto hit_mod = fmin(0.5f, hit + vel * vel_hit * 0.5f);
	auto amp_k = 35.0f * si_fabsf(fastersinfullf(M_PI * k * hit_mod));

	// Bandpass filter coefficients (normalized)
	b0 = alpha * tone_gain * amp_k;
	// b2 = -alpha * tone_gain * amp_k;
	b2 = -b0;
	a0 = decay_k > c_decay_min ? 1.0f + alpha / decay_k : 0.0f;
	a1 = -2.0f * fastercosfullf(omega);
	a2 = decay_k > c_decay_min ? 1.0f - alpha / decay_k : 0.0f;
}

float32x4_t Partial::process(float32x4_t input)
{
	// Second-order IIR bandpass filter: output = (b0*x + b2*x2 - a1*y1 - a2*y2) / a0
	// Process 4 samples using NEON vectorization
    float32x4_t a0_vec = vdupq_n_f32(a0);
	float32x4_t b0_vec = vdupq_n_f32(b0);
	float32x4_t b2_vec = vdupq_n_f32(b2);
	float32x4_t a1_vec = vdupq_n_f32(a1);
	float32x4_t a2_vec = vdupq_n_f32(a2);

	// Numerator: b0*x + b2*x2
    float32x4_t num = vmlaq_f32(vmulq_f32(x2, b2_vec), input, b0_vec);
    // Feedback: a1*y1 + a2*y2
    float32x4_t denom_corr = vmlaq_f32(vmulq_f32(y2, a2_vec), y1, a1_vec);
	// Final: (numerator - feedback) / a0 using reciprocal approximation for audio fidelity
	// Safe because a0 is validated as non-zero in update()
	float32x4_t recip_a0 = vrecpeq_f32(a0_vec);
	float32x4_t output = vmulq_f32(vsubq_f32(num, denom_corr), recip_a0);

	x2 = x1;
	x1 = input;
	y2 = y1;
	y1 = output;

	return output;
}

void Partial::clear()
{
	x1 = x2 = y1 = y2 = vdupq_n_f32(0.0f);
}