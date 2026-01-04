#include "Resonator.h"
// The resonator, as partial or waveguide, is the main body that's vibrating,
// so it's the core of the sound emitted at note /

Resonator::Resonator()
{
	for (size_t i = 0; i < c_max_partials; ++i) {
		 partials.push_back(Partial(i + 1));
	}
}

// called by original plugin::onSlider() => TODO
void Resonator::setParams(float32_t _srate, bool _on, int _model, int _partials, float32_t _decay,
    float32_t _damp, float32_t tone, float32_t hit,	float32_t _rel, float32_t _inharm, float32_t _cut,
    float32_t _radius, float32_t vel_decay, float32_t vel_hit, float32_t vel_inharm)
{
	on = _on;
	nmodel = _model;
	npartials = _partials;
	decay = _decay;
	radius = _radius;
	srate = _srate;
	cut = _cut;

	auto freq = 20.0 * fasterpowf(20000.0 / 20.0, cut < 0.0 ? 1 + cut : cut); // map 1..0 to 20..20000, with inverse scale for negative norm;
	if (_cut < 0.0) {
		filter.lp(srate, freq, 0.707);
	}
	else {
		filter.hp(srate, freq, 0.707);
    }
	for (Partial& partial : partials) {
		partial.damp = _damp;
		partial.decay = decay;
		partial.hit = hit;
		partial.inharm = _inharm;
		partial.rel = _rel;
		partial.tone = tone;
		partial.vel_decay = vel_decay;
		partial.vel_hit = vel_hit;
		partial.vel_inharm = vel_inharm;
		partial.srate = _srate;
	}

	waveguide.decay = decay;
	waveguide.radius = vmov_n_f32(radius);
	waveguide.is_closed = _model == ModelNames::ClosedTube;
	waveguide.srate = srate;
	waveguide.vel_decay = vel_decay;
	waveguide.rel = _rel;
}

void Resonator::update(float32_t freq, float32_t vel, bool isRelease, std::array<float32_t,64> model)
{
	if (nmodel < OpenTube) {
		for (Partial& partial : partials) {
			auto idx = partial.k - 1; // clears lnt-arithmetic-overflow warning when accessing _model[k-1] directly
			partial.update(freq, model[idx], model[model.size() - 1], vel, isRelease);
		}
	}
	else
		waveguide.update(model[0] * freq, vel, isRelease);
}

void Resonator::activate()
{
	active = true;
	silence = 0;
}

// input is two stereo samples / four mono samples
float32x4_t Resonator::process(float32x4_t input)
{
	float32x4_t out = vdupq_n_f32(0.0);

	if (active) { // use active and silence to turn off strings process if not in use
		if (nmodel < 7) {
			for (int p = 0; p < npartials; ++p) {
				out = vaddq_f32(out, partials[p].process(input));
			}
		}
		else
			out = vaddq_f32(out, waveguide.process(input)); // waveguide process
	}

	// if (fabs(out) + fabs(input) > 0.00001)
	float32x4_t result = vaddq_f32(vabsq_f32(out), vabsq_f32(input));
	silence += vgetq_lane_f32(result, 0) > c_silence_threshold ? 1 : 0;
	silence += vgetq_lane_f32(result, 1) > c_silence_threshold ? 1 : 0;
	silence += vgetq_lane_f32(result, 2) > c_silence_threshold ? 1 : 0;
	silence += vgetq_lane_f32(result, 3) > c_silence_threshold ? 1 : 0;
	if (silence >= srate)
		active = false;

	return out;
}

void Resonator::clear()
{
	for (Partial& partial : partials) {
		partial.clear();
	}
	waveguide.clear();
	filter.clear(0.0);
}
