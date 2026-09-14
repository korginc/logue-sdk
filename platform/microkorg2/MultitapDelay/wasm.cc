// websim host bridge for microKORG2 "MultitapDelay" delay effect.
// All logic lives in the shared bridge; this just selects the DSP class.
#include "MultitapDelay.h"
#define MK2_FX_CLASS MultitapDelay
#include "mk2_fx_bridge.h"
