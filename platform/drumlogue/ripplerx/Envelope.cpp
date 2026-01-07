#include "float_math.h"
#include "Envelope.h"

// Normalize tension from [-1,1] to [0.001..1, 100 (linear), 2..1]
float32_t Envelope::normalizeTension(float32_t t)
{
	t += 1.0;
	return t == 1.0 ? 100.0
		: t > 1.0 ? 3.001 - t : 0.001 + t;
}

void Envelope::init(float32_t srate, float32_t a, float32_t d, float32_t s, float32_t r, float32_t tensionA, float32_t tensionD, float32_t tensionR) {
	att = fmax(a, 1.0) * 0.001 * srate;
	dec = fmax(d, 1.0) * 0.001 * srate;
	sus = fasterpowf(10.0f, fmin(s, 0.0f) / 20.0f);
	rel = fmax(r, 1.0) * 0.001 * srate;

	ta = normalizeTension(tensionA);
	td = normalizeTension(-1.0 * tensionD);
	tr = normalizeTension(-1.0 * tensionR);
}

void Envelope::calcCoefs(float32_t targetB1, float32_t targetB2, float32_t targetC, float32_t rate,
    float32_t tension, float32_t mult, float32_t& result_b, float32_t& result_c)
{
	float32_t t;
	if (tension > 1.0) {  // slow-start shape
		t = fasterpowf(tension - 1, 3.0);
		// c = pow(log((targetC + t) / t) / rate);
		result_c = e_expf(fasterlogf((targetC + t) / t) / rate);
		result_b = ((targetB1 - mult * t) * (1 - result_c));
	} else {			  // fast-start shape (inverse exponential)
		t = fasterpowf(tension, 3);
		// c = pow(-log((targetC + t) / t) / rate);
		result_c = e_expf(-fasterlogf((targetC + t) / t) / rate);
		result_b = ((targetB2 + mult * t) * (1 - result_c));
	}
}

void Envelope::recalcCoefs()
{
	// calculate attack coefficients
	calcCoefs(0.0, scale, scale, att, ta, 1.0, ab, ac);
	// calculate decay coefficients
	calcCoefs(1.0, sus*scale, (1.0-sus)*scale, dec, td, -1.0, db ,dc);
}

void Envelope::reset()
{
	state = Off;
	env = 0.0;
}

void Envelope::attack(float32_t _scale)
{
	scale = _scale;
	recalcCoefs();
	state = Attack;
}

void Envelope::decay()
{
	env = scale;
	state = Decay;
}

void Envelope::sustain()
{
	env = scale * sus;
	state = Sustain;
}

void Envelope::release()
{
	calcCoefs(fmax(env, sus) * scale, 0.0,
                   fmax(env, sus) * scale,
                   rel, tr, -1.0, rb, rc);
	state = Release;
}

int Envelope::process()
{
	if (!state) return 0;

	if (state == Attack) {
		env = ab + env * ac;
		if (env >= scale) decay();
	}

	else if (state == Decay) {
		env = db + env * dc;
		if (env <= sus * scale) sustain();
	}

	else if (state == Sustain) {
		// Sustain state: hold envelope at sustain level until release is called
		return state;
	}

	else if (state == Release) {
		env = rb + env * rc;
		if (env <= 0) reset();
	}

	return state;
}

