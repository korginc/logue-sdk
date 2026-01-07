// Copyright 2025 tilr
// Waveguide for OpenTube and ClosedTube models
// Uses float32x4_t ARM NEON vectors for consistency with rest of codebase
#pragma once
#include <vector>
#include "float_math.h"

class Waveguide
{
public:
	Waveguide();  // Proper initialization in .cpp
	~Waveguide() {};

	void update(float32_t freq, float32_t vel, bool isRelease);
	float32x4_t process(float32x4_t input);  // Process 4 samples (stereo pair) in parallel
	void clear();

	bool is_closed = false;
	float32_t srate = 0.0f;
	float32_t decay = 0.0f;
	float32x4_t radius{};      // Zero-initialized (applied to all 4 samples)
	float32x4_t max_radius{};  // Zero-initialized
	float32_t rel = 0.0f;
	float32_t vel_decay = 0.0f;

private:
	int read_ptr = 0;
	int write_ptr = 0;
	float32_t tube_decay = 0.0f;
	std::vector<float32x4_t> tube;  // Changed to float32x4_t for NEON vectorization
	static constexpr int tube_len = 20000;  // buffer size, 20000 allows for 10Hz at 200k srate

	float32x4_t y{};   // Zero-initialized, proper member
	float32x4_t y1{};  // Zero-initialized, proper member
};