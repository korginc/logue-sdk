#include "Resonator.h"
// The resonator, as partial or waveguide, is the main body that's vibrating,
// so it's the core of the sound emitted at note /

Resonator::Resonator()
{
	for (size_t i = 0; i < c_max_partials; ++i) {
		 partials.push_back(Partial(i + 1));
	}
}

// called by original plugin::onSlider()
void Resonator::setParams(float32_t _srate, bool _on, int _model, int _partials, float32_t _decay,
    float32_t _damp, float32_t tone, float32_t hit,	float32_t _rel, float32_t _inharm, float32_t _cut,
    float32_t _radius, float32_t vel_decay, float32_t vel_hit, float32_t vel_inharm)
{
	on = _on;
	validateAndSetModel(_model);
	validateAndSetPartials(_partials);
	decay = _decay;
	radius = _radius;
	srate = _srate;
	cut = _cut;

	// Map cut parameter: 1..0 to 20..20000 Hz with inverse scale for negative values
	// Ratio = 20000/20 = 1000, so freq = 20 * 1000^exponent
	constexpr float32_t f_min = 20.0f;
	constexpr float32_t f_max = 20000.0f;
	constexpr float32_t f_ratio = 1000.0f;  // f_max / f_min
	constexpr float32_t q_butterworth = 0.707f;  // Butterworth Q = sqrt(2)/2 for maximally flat response

	auto freq = f_min * fasterpowf(f_ratio, cut < 0.0f ? 1.0f + cut : cut);

	if (_cut < 0.0f) {
		filter.lp(srate, freq, q_butterworth);
	}
	else {
		filter.hp(srate, freq, q_butterworth);
    }

	// Update ONLY the active partials, not all 64
	for (int p = 0; p < npartials; ++p) {
		partials[p].damp = _damp;
		partials[p].decay = decay;
		partials[p].hit = hit;
		partials[p].inharm = _inharm;
		partials[p].rel = _rel;
		partials[p].tone = tone;
		partials[p].vel_decay = vel_decay;
		partials[p].vel_hit = vel_hit;
		partials[p].vel_inharm = vel_inharm;
		partials[p].srate = _srate;
	}

	waveguide.decay = decay;
	waveguide.radius = vmov_n_f32(radius);
	waveguide.is_closed = _model == ModelNames::ClosedTube;
	waveguide.srate = srate;
	waveguide.vel_decay = vel_decay;
	waveguide.rel = _rel;
}

void Resonator::validateAndSetModel(int _model)
{
	// Clamp model to valid range [0, 7] for different resonator types
	// See ModelNames enum for valid values
	if (_model < 0) nmodel = 0;
	else if (_model > OpenTube) nmodel = OpenTube;
	else nmodel = _model;
}

void Resonator::validateAndSetPartials(int _partials)
{
	// Clamp to valid partial count [1, 64]
	if (_partials < 1) npartials = 1;
	else if (_partials > static_cast<int>(c_max_partials)) npartials = static_cast<int>(c_max_partials);
	else npartials = _partials;
}

void Resonator::update(float32_t freq, float32_t vel, bool isRelease, std::array<float32_t,64> model)
{
	if (nmodel < OpenTube) {
		// Update each partial with validated bounds
		for (size_t p = 0; p < partials.size() && p < static_cast<size_t>(npartials); ++p) {
			auto idx = partials[p].k - 1; // partial number to array index
			// Bounds check: ensure idx is within valid range [0, 63]
			if (idx >= 0 && idx < static_cast<int>(model.size())) {
				partials[p].update(freq, model[idx], model[model.size() - 1], vel, isRelease);
			}
		}
	}
	else if (model.size() > 0) {
		// Waveguide: use first model value as frequency multiplier
		waveguide.update(model[0] * freq, vel, isRelease);
	}
}

void Resonator::activate()
{
	if (!active) {  // Only reset silence counter on state change
		active = true;
		silence = 0;
	}
}

// input is two stereo samples / four mono samples
float32x4_t Resonator::process(float32x4_t input)
{
	float32x4_t out = vdupq_n_f32(0.0f);

	if (active) { // use active and silence to turn off strings process if not in use
		if (nmodel < OpenTube) {
			// Process through partials with validated bounds
			for (size_t p = 0; p < partials.size() && p < static_cast<size_t>(npartials); ++p) {
				out = vaddq_f32(out, partials[p].process(input));
			}
		}
		else {
			// Process through waveguide
			out = vaddq_f32(out, waveguide.process(input));
		}
	}

	// Track silence: count samples below threshold
	float32x4_t result = vaddq_f32(vabsq_f32(out), vabsq_f32(input));
	silence += vgetq_lane_f32(result, 0) > c_silence_threshold ? 1 : 0;
	silence += vgetq_lane_f32(result, 1) > c_silence_threshold ? 1 : 0;
	silence += vgetq_lane_f32(result, 2) > c_silence_threshold ? 1 : 0;
	silence += vgetq_lane_f32(result, 3) > c_silence_threshold ? 1 : 0;

	// Deactivate if silent for ~1 second
	if (silence >= static_cast<int>(srate)) {
		active = false;
	}

	return out;
}

void Resonator::clear()
{
	for (Partial& partial : partials) {
		partial.clear();
	}
	waveguide.clear();
	filter.clear(0.0f);
	silence = 0;  // Reset silence counter
}
