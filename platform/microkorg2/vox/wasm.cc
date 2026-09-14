// websim host bridge for microKORG2 "vox" oscillator.
// All logic lives in the shared bridge; this just selects the DSP class.
// vox uses a few NEON intrinsics directly — the build force-includes
// websim/dsp/microkorg2/mk2_simd_compat.h to provide them (see WEBSIM.md).
#include "vox.h"
#define MK2_OSC_CLASS Vox
#define MK2_OSC_VOICES 4  // small polyphony: one x4 voice group (see WEBSIM.md §D)
#include "mk2_osc_bridge.h"
