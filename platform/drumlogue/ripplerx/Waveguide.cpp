#include "Waveguide.h"

void Waveguide::update(float32_t f_0, float32_t vel, bool isRelease)
{
	auto tlen = srate / f_0;
	if (is_closed) tlen *= 0.5; // fix closed tube one octave lower
	read_ptr = (int)(write_ptr - tlen + tube_len) % tube_len;

	// auto decay_k = fmin(100.0, exp(log(decay) + vel * vel_decay * (log(100) - log(0.01))));
    // where (log(100) - log(0.01)) is 2*log(10) - (-2*log(10)) = 2Log(100)
	auto decay_k = fmin(100.0, e_expff(fasterlogf(decay) + (vel * vel_decay * M_TWOLN100)));
	if (isRelease) decay_k = rel * decay_k;
	tube_decay = decay_k
    // ? exp(-juce::MathConstants<double>::pi / f_0 / (srate * decay_k / 125000))
    // 125000 set by hear so that decay approximates in seconds.
		? e_expff((-M_PI * 125000) / (f_0 * srate * decay_k))
		: 0.0;
}

float32x2_t Waveguide::process(float32x2_t input)
{
	// auto sample = tube[read_ptr];
	float32x2_t sample = tube[read_ptr]; //load two values, as input is stereo
	// Apply lowpass filter for frequency damping (tube radius)
    // y = radius * sample + (1.0 - radius) * y1;
	y = vmla_f32(vmul_f32(radius, sample), y1, vsub_s32(max_radius, radius));
	y1 = y;

	// Apply decay to sample
	auto dsample = vmul_n_f32(y, tube_decay);
	// if (is_closed) dsample *= -1.0; // closed tube, inverting dsample causes only odd harmonics
	if (is_closed) // closed tube, inverting dsample causes only odd harmonics
	    tube[write_ptr] = vsub_s32(input, dsample);
    else
	    tube[write_ptr] = vadd_u32(input, dsample);

	write_ptr = (write_ptr + 1) % tube_len;
	read_ptr = (read_ptr + 1) % tube_len;

	return dsample;
}

void Waveguide::clear()
{
	y = y1 = vmov_n_f32(0.0);
    read_ptr = write_ptr = 0;
    tube.clear();
    tube.resize(tube_len);
}