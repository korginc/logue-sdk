// websim host bridge for microKORG2 "Vibrato" modulation effect.
// All logic lives in the shared bridge; this just selects the DSP class.
#include "Vibrato.h"
#define MK2_FX_CLASS Vibrato
#include "mk2_fx_bridge.h"
