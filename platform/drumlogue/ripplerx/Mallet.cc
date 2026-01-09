#include "Mallet.h"

void Mallet::trigger(/** TODO:MalletType _type, */float32_t srate, float32_t freq)
{
	/** TODO:
	type = _type;
	srate = _srate;

	if (type == kImpulse) { */
		filter.bp(srate, freq, 0.707);
		filter.reset();
		elapsed = (int)(srate / 10.0); // countdown for 100ms
		impulse = 2.0; // Pre-multiply amplitude
		// Envelope decay coefficient for 100ms decay time
		env = e_expff(-1.0 / (0.1f * srate));
	//}
	// else {
	// 	playback_speed = sampler.wavesrate / srate;
	// 	playback = 0.0;
	// }
}

void Mallet::clear()
{
	elapsed = 0;
	impulse = 0.0;
	filter.reset();
	// TODO:
	// playback = INFINITY;
	// impulse_filter.clear(0.0);
	// sample_filter.clear(0.0);
}