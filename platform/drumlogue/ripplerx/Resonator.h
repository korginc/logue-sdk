// Copyright 2025 tilr
// Resonator holds a number of Partials and a Waveguide
// depending on the selected model uses the Partials bank or Waveguide to process input
// the partials are tuned by selected model by Voice.h

#pragma once
#include <vector>
#include "Partial.h"
#include "Waveguide.h"
#include "Filter.h"
#include "Models.h"
#include "constants.h"


/** main class */
class Resonator
{
public:
	Resonator();
	~Resonator() {};

	void setParams(float32_t _srate, bool _on, int _model, int _partials, float32_t _decay,
        float32_t _damp, float32_t tone, float32_t hit,	float32_t _rel, float32_t _inharm, float32_t _cut,
        float32_t _radius, float32_t vel_decay, float32_t vel_hit, float32_t vel_inharm);
	void activate();
	void clear();
	void update(float32_t frequency, float32_t vel, bool isRelease, std::array<float32_t, 64> _model);
	float32x4_t process(float32x4_t input);

	// Public read-only accessors for state
	bool isActive() const { return active; }
	int getModel() const { return nmodel; }
	int getPartialCount() const { return npartials; }
	int getSilenceCounter() const { return silence; }
	bool isOn() const { return on; }

private:
	// State members - protected from external modification
	int silence = 0; // counter of samples of silence
	bool active = false; // returns to false if samples of silence run for a bit
	float32_t srate = 0.0f;
	bool on = false;
	int nmodel = 0;
	int npartials = 0;
	float32_t decay = 0.0f;
	float32_t radius = 0.0f;
	float32_t cut = 0.0f;

	std::vector<Partial> partials;
	Waveguide waveguide{};
	Filter filter{};

	// Validation helpers
	void validateAndSetPartials(int _partials);
	void validateAndSetModel(int _model);
};

