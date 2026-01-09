// Copyright 2025 tilr
// Port of Envlib (tilr) based of zenvelib.v1 by dwelx
// provides adsr envelopes with tension/shape and scale controls
#pragma once
#include <arm_neon.h>

enum envelope_state : int
{
    Off = 0,
    Attack = 1,
    Decay = 2,
    Sustain = 3,
    Release = 4
};

class Envelope
{
public:
	Envelope() {};
	~Envelope() {};

	void init(float32_t srate, float32_t a, float32_t d, float32_t s, float32_t r, float32_t tensionA,
        float32_t tensionD, float32_t tensionR);
	void reset();
	void attack(float32_t scale);
	void release();
	void sustain();
	void decay();
	int process();

	// Getters for read-only access to state
	float32_t getEnv() const { return env; }
	envelope_state getState() const { return state; }

protected:
	float32_t att = 0.0;
	float32_t dec = 0.0;
	float32_t sus = 0.0;
	float32_t rel = 0.0;
	float32_t scale = 0.0;
	float32_t env = 0.0;
	envelope_state state = Off;

	float32_t ab = 0.0; // attack coeff
	float32_t ac = 0.0; // attack coeff
	float32_t db = 0.0; // decay coeff
	float32_t dc = 0.0; // decay coeff
	float32_t rb = 0.0; // release coeff
	float32_t rc = 0.0; // release coeff
	float32_t ta = 0.0; // tension att
	float32_t td = 0.0; // tension dec
	float32_t tr = 0.0; // tension rel

private:
	// Normalize tension from [-1,1] to [0.001..1, 100 (linear), 2..1]
	inline float32_t normalizeTension(float32_t t) {
		t += 1.0f;
		return t == 1.0f ? 100.0f
			: t > 1.0f ? 3.001f - t : 0.001f + t;
	}

	void calcCoefs(float32_t targetB1, float32_t targetB2, float32_t targetC, float32_t rate,
        float32_t tension, float32_t mult, float32_t& result_b, float32_t& result_c);
	void recalcCoefs(); // calcs coefficients for attack and decay
};