// Copyright (C) 2025 tilr
// Noise generator with a filter and envelope
#pragma once
#include "WFLCG.hh"
#include "float_math.h"
#include "Filter.h"
#include "Envelope.h"

/** Using random number generator from:
 *  https://github.com/WarpRules/WFLCG
 */
class Noise
{
public:
	Noise() {};
	~Noise() {};

	void init(float32_t _srate,
		int filterMode,
		float32_t _freq,
        float32_t _q,
		float32_t att,
		float32_t dec,
        float32_t sus,
		float32_t rel,
		// these two values are not present in the original preset/*.xml files
		float32_t _vel_freq,
        float32_t _vel_q);
	inline float32_t process() {
		// Early exit if envelope is not active
		if (!env.getState()) return 0.0f;

		env.process();

		// Generate noise sample in range [-1.0, 1.0)
		// WFLCG::getFloat() returns [1.0, 2.0), so multiply by 2.0 and subtract 3.0
		// Result: [1.0*2.0 - 3.0, 2.0*2.0 - 3.0) = [-1.0, 1.0)
		float32_t sample = rng.getFloat() * 2.0f - 3.0f;

		if (filter_active)
			sample = filter.df1(sample);

		// Apply envelope and check if finished
		float32_t env_val = env.getEnv();

		// Clear filter state when envelope finishes to avoid pops
		if (!env.getState())
			filter.clear(0.0f);

		return sample * env_val;
	}
	void attack(float32_t vel);
	void initFilter();
	void release();
	void clear();

	float32_t vel_freq = 0.0;
	float32_t vel_q = 0.0;
	float32_t srate = 44100.0;
	float32_t vel = 0.0;
	float32_t q = 0.707;
	bool filter_active = false;
private:
	Filter filter{};
	Envelope env{};
	WFLCG rng{};		// Persistent RNG instance - initialized once
	int fmode = 0;
	float32_t freq = 0.0;
};