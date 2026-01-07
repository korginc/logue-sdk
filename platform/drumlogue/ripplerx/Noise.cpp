#include "Noise.h"
#include "constants.h"

void Noise::init(float32_t _srate, int filterMode, float32_t _freq, float32_t _q, float32_t att, float32_t dec, float32_t sus, float32_t rel, float32_t _vel_freq, float32_t _vel_q)
{
	srate = _srate;
	fmode = filterMode;
	freq = _freq;
	q = _q;
	vel_freq = _vel_freq;
	vel_q = _vel_q;
	initFilter();
	env.init(srate, att, dec, sus, rel, 0.4, 0.4, 0.4);
}

void Noise::attack(float32_t _vel)
{
	vel = _vel;
	initFilter();
	env.attack(1.0);
}

void Noise::initFilter()
{
	// Compute velocity-modulated frequency using pre-computed log constants
	float32_t log_f = fasterlogf(freq) + vel * vel_freq * c_noise_filter_log_range;
	float32_t f = e_expff(log_f);
	// Clamp frequency to valid range [20, 20000]
	f = fmin(c_noise_filter_freq_max, fmax(c_noise_filter_freq_min, f));

	// Compute velocity-modulated resonance, clamped to [0.707, 4.0]
	float32_t res = q + vel * vel_q * c_noise_filter_res_range;
	filter_active = fmode == 1 || (fmode == 0 && f < 20000.0f) || (fmode == 2 && f > 20.0f);

	if (fmode == 0) filter.lp(srate, f, res);
	else if (fmode == 1) filter.bp(srate, f, res);
	else if (fmode == 2) filter.hp(srate, f, res);
	// Invalid filter mode - clamp to LP by default instead of throwing
}

void Noise::release()
{
	env.release();
}

void Noise::clear()
{
	env.reset();
	filter.clear(0.0f);
}

inline float32_t Noise::process()
{
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
