#include "Waveguide.h"

// Proper constructor with safe initialization
Waveguide::Waveguide()
	: read_ptr(0), write_ptr(0), tube_decay(0.0f), y{}, y1{}
{
	// Initialize radius and max_radius using NEON intrinsics for float32x4_t
	// Broadcast single values to all 4 lanes
	radius = vdupq_n_f32(0.0f);
	max_radius = vdupq_n_f32(1.0f);
	tube.resize(tube_len);
}

void Waveguide::update(float32_t f_0, float32_t vel, bool isRelease)
{
	// Validate frequency to avoid division by zero
	if (f_0 < 20.0f) f_0 = 20.0f;  // Minimum 20Hz

	auto tlen = srate / f_0;
	if (is_closed) tlen *= 0.5f; // fix closed tube one octave lower

	// Safe modulo: ensure tube_len is never zero
	if (tube_len > 0) {
		read_ptr = (int)(write_ptr - tlen + tube_len) % tube_len;
		if (read_ptr < 0) read_ptr += tube_len;  // Handle negative modulo
	}

	// auto decay_k = fmin(100.0f, exp(log(decay) + vel * vel_decay * (log(100.0f) - log(0.01f))));
	// where (log(100.0f) - log(0.01f)) is 2*log(10) - (-2*log(10)) = 2Log(100)
	auto decay_k = fmin(100.0f, e_expff(fasterlogf(decay) + (vel * vel_decay * M_TWOLN100)));
	if (isRelease) decay_k = rel * decay_k;

	// Safe decay calculation with threshold
	tube_decay = decay_k > 0.0f
		? e_expff((-M_PI * 125000.0f) / (f_0 * srate * decay_k))
		: 0.0f;  // ✅ Explicit float suffix
}

float32x4_t Waveguide::process(float32x4_t input)
{
	// Bounds check to prevent buffer overrun
	if (read_ptr < 0 || read_ptr >= static_cast<int>(tube.size())) {
		read_ptr = 0;  // Safety reset
	}
	if (write_ptr < 0 || write_ptr >= static_cast<int>(tube.size())) {
		write_ptr = 0;  // Safety reset
	}

	// Load 4 values (stereo pair), using float32x4_t operations
	float32x4_t sample = tube[read_ptr];

	// Apply lowpass filter for frequency damping (tube radius)
	// y = radius * sample + (1.0f - radius) * y1
	// Using vsubq_f32 for 128-bit subtraction: one_minus_radius = [1, 1, 1, 1] - radius
	float32x4_t one_minus_radius = vsubq_f32(vdupq_n_f32(1.0f), radius);
	
	// vmlaq_f32: result = accumulator + (operand1 * operand2)
	// y = (radius * sample) + ((1.0f - radius) * y1)
	y = vmlaq_f32(vmulq_f32(radius, sample), y1, one_minus_radius);
	y1 = y;

	// Apply decay to sample
	float32x4_t dsample = vmulq_n_f32(y, tube_decay);

	// Closed tube inverts harmonics (only odd harmonics pass)
	if (is_closed) {
		// Proper float subtraction: output = input - dsample (inverted)
		tube[write_ptr] = vsubq_f32(input, dsample);
	}
	else {
		// Proper float addition: output = input + dsample
		tube[write_ptr] = vaddq_f32(input, dsample);
	}

	// Advance pointers with safe wrapping
	write_ptr = (write_ptr + 1) % tube_len;
	if (write_ptr < 0) write_ptr = 0;
	
	read_ptr = (read_ptr + 1) % tube_len;
	if (read_ptr < 0) read_ptr = 0;

	return dsample;
}

void Waveguide::clear()
{
	y = vdupq_n_f32(0.0f);
	y1 = vdupq_n_f32(0.0f);
	read_ptr = 0;
	write_ptr = 0;

	// Clear and resize buffer safely
	tube.clear();
	tube.resize(tube_len);
}