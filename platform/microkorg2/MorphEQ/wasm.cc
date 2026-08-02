// websim host bridge for microKORG2 "MorphEQ" modulation effect (EQ).
// All logic lives in the shared bridge; this just selects the DSP class.
#include "MorphEQ.h"
#define MK2_FX_CLASS MorphEQ
#define MK2_FX_RENDER Render  // MorphEQ names its render method Render, not Process
#include "mk2_fx_bridge.h"
