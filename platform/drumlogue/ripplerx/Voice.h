// Copyright 2025 tilr

// Each pressed key triggers a voice, there are max 18 polyphony voices in Drumlogue
// Voices hold A and B resonators, a mallet and noise generator
// they also calculate split frequencies for coupled resonators
// and tune the resonators modals by providing the models

#include <array>
#include "Mallet.h"
#include "Noise.h"
#include "Resonator.h"
#include "tuple"

class Models;

class Voice
{
public:
	Voice(Models& m) : models(m) {}
	~Voice() {}

	float32_t note2freq(int _note);
	void trigger(float32_t srate, int _note, float32_t _vel, float32_t malletFreq);
	void release();
	void clear();
	void setPitch(float32_t a_coarse, float32_t b_coarse, float32_t a_fine, float32_t b_fine);
	void applyPitch(std::array<float32_t, 64>& model, float32_t factor);
	float32x4_t inline freqShift(float32_t fa, float32_t fb) const;
	std::tuple<std::array<float32_t, 64>, std::array<float32_t, 64>> calcFrequencyShifts();
	void setCoupling(bool _couple, float32_t _split);
	void updateResonators();

	int note = 0;
	float32_t freq = 0.0;
	float32_t vel = 0.0;
	bool isRelease = false;
	bool isPressed = false; // used for audioIn
	bool couple = false;
	float32_t split = 0.0;

	float32_t aPitchFactor = 1.0;
	float32_t bPitchFactor = 1.0;

	Mallet    mallet{};
	Noise     noise{};
	Resonator resA{};
	Resonator resB{};

private:
	Models& models;
};
