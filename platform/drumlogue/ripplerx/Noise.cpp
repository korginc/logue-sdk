#include "Noise.h"

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
	float32_t f = fmin(20000.0,
                      fmax(20.0,
                           e_expff(fasterlogf(freq) +
                                   vel * vel_freq * (fasterlogf(20000.0) - fasterlogf(20.0)))));
	float32_t res = fmin(4.0, fmax(0.707, q + vel * vel_q * (4.0 - 0.707)));

	filter_active = fmode == 1 || (fmode == 0 && f < 20000.0) || (fmode == 2 && f > 20.0);

	if (fmode == 0) filter.lp(srate, f, res);
	else if (fmode == 1) filter.bp(srate, f, res);
	else if (fmode == 2) filter.hp(srate, f, res);
	else throw "Unknown filter mode";
}

void Noise::release()
{
	env.release();
}

void Noise::clear()
{
	env.reset();
	filter.clear(0.0);
}

float32_t Noise::process()
{
	if (!env.state) return 0.0;
	env.process();
	// float32_t sample = (std::rand() / (float32_t)RAND_MAX) * 2.0 - 1.0;
    WFLCG rng;
	float32_t sample = rng.getFloat() - 2.0;  // -1.0 < x < +1.0
	if (filter_active)
		sample = filter.df1(sample);

	if (!env.state)
		filter.clear(0.0); // envelope has finished, clear filter to avoid pops

	return sample * env.env;
}

