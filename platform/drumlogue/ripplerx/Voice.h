// Copyright 2025 tilr

// Each pressed key triggers a voice, I set max 8 polyphony voices in Drumlogue
// Voices hold A and B resonators, a mallet and noise generator
// they also calculate split frequencies for coupled resonators
// and tune the resonators modals by providing the models

#include <array>
#include "Mallet.h"
#include "Noise.h"
#include "Resonator.h"
#include "tuple"

class Models;
//TODO: class Sampler

class Voice
{
public:
	Voice(Models& m/**< TODO:, Sampler& s */) : models(m) {} /**<TODO: , mallet(s) {} */
	~Voice() {}

	float32_t note2freq(int _note);
	void trigger(/**uint64_t timestamp,*/ float32_t srate, int _note,
		float32_t _vel, /**MalletType malletType, */float32_t malletFreq/** TODO:,
		float32_t malletKTrack, bool skip_fade*/);
	//TODO:
	//void triggerStart(bool reset);
	//float32_t fadeOut();
	void release(/**uint64_t timestamp*/);
	void clear();
	void setPitch(float32_t a_coarse, float32_t b_coarse, float32_t a_fine, float32_t b_fine);
	void setRatio(float32_t _a_ratio, float32_t _b_ratio);
	void applyPitch(std::array<float32_t, 64>& model, float32_t factor);
	float32x4_t inline freqShift(float32_t fa, float32_t fb) const;
	std::tuple<std::array<float32_t, 64>, std::array<float32_t, 64>> calcFrequencyShifts();

	/** TODO:
	double processOscillators(bool isA);


	*/
	void updateResonators();
	void setCoupling(bool _couple, float32_t _split);
	inline size_t getFramesSinceNoteOn() const {
		if (!m_initialized) return SIZE_MAX;
		return m_framesSinceNoteOn;
	}
	int       note = 0;
	float32_t freq = 0.0;
	float32_t vel = 0.0;
	bool      isRelease = false;
	bool      isPressed = false; // used for audioIn
	bool      couple = false;
	//TODO: float32_t malletKtrack = 0.0;
	float32_t split = 0.0;
	/**< TODO:
	float32_t srate = 44100.0;
	float32_t a_ratio = 1.0; // used to recalculate models
	float32_t b_ratio = 1.0; // used to recalculate models
	uint64_t  pressed_ts = 0; // timestamp used to order notes
	uint64_t  release_ts = 0; // timestamp used to order notes

	// used to fade out on repeat notes
	bool      isFading = false;
	int       fadeTotalSamples = 0;
	int       fadeSamples = 0;
	MalletType malletType = kImpulse;
	float32_t malletFreq = 0.0;
	float32_t newFreq = 0.0;
	float32_t newVel = 0.0;
	int       newNote = 0;
	*/
	bool      m_initialized = false;
	bool      m_gate = false;
	size_t    m_framesSinceNoteOn = SIZE_MAX; // Voice stealing
	float32_t aPitchFactor = 1.0;
	float32_t bPitchFactor = 1.0;

	Mallet    mallet{};
	Noise     noise{};
	Resonator resA{};
	Resonator resB{};

private:
	Models& models;
	//TODO:
	//std::array<float32_t, 64> aPhases = {};
	//std::array<float32_t, 64> bPhases = {};
};
