// websim host bridge for microKORG2 "waves" oscillator.
// All logic lives in the shared bridge; this just selects the DSP class.
#include "waves.h"
#define MK2_OSC_CLASS Waves
#include "mk2_osc_bridge.h"
