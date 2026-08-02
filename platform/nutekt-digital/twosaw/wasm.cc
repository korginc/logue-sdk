// websim host bridge for the gen-1 "twosaw" oscillator (manifest-driven params).
// The param table is generated from manifest.json (websim_legacy_params.h, via
// the Makefile's WEBSIM_LEGACY_MANIFEST); the shared legacy bridge appends the
// SHAPE / SHIFT-SHAPE knobs and drives OSC_INIT/CYCLE/PARAM. See WEBSIM.md §C.
#include "userosc.h"
#include "websim_legacy_params.h"
#include "legacy_osc_bridge.h"
