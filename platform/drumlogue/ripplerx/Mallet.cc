#include "Mallet.h"

void Mallet::trigger(/** TODO:MalletType _type, */float32_t srate, float32_t freq)
{
	/** TODO:
	type = _type;
	srate = _srate;

	if (type == kImpulse) { */
		filter.bp(srate, freq, 0.707f);
		filter.reset();
		elapsed = (int)(srate / 10.0f); // countdown for 100ms
		impulse = 2.0f; // Pre-multiply amplitude
		// Envelope decay coefficient for 100ms decay time
		if (srate > 0.0f)
			env = e_expff(-1.0f / (0.1f * srate));
	//}
	// else {
	// 	playback_speed = sampler.wavesrate / srate;
	// 	playback = 0.0;
	// }
}

void Mallet::clear()
{
	elapsed = 0;
	impulse = 0.0f;
	filter.reset();
	// TODO:
	// playback = INFINITY;
	// impulse_filter.clear(0.0);
	// sample_filter.clear(0.0);
}
