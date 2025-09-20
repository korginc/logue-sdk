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

	void init(float32_t _srate, int filterMode, float32_t _freq,
        float32_t _q, float32_t att, float32_t dec,
        float32_t sus, float32_t rel, float32_t _vel_freq,
        float32_t _vel_q);
	float32_t process();
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
	int fmode = 0;
	float32_t freq = 0.0;
};