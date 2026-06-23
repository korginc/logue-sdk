// websim host bridge for the drumlogue "SmplVox" sample-playback voice.
// All logic lives in the shared bridge; this just selects the DSP class. The
// bridge provides a synthetic sample bank (dl_sample_bank.h) so the voice has
// something to play in the browser. See WEBSIM.md / WEBSIM_FOLLOWUP_PLAN.md §C.4.
#include "synth.h"
#define DL_SYNTH_CLASS Synth
#include "dl_synth_bridge.h"
