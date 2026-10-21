##############################################################################
# websim gen-1 (legacy) build fragment
#
# Gen-1 oscillators (prologue / minilogue xd / NTS-1 mkI / nutekt-digital) expose
# their DSP as free C functions (OSC_INIT/OSC_CYCLE/OSC_PARAM) emitting q31, not
# as a C++ class. The host bridge differs (see websim/dsp/legacy/), but the emcc
# recipe itself is the shared one. A gen-1 project sets the WEBSIM_* variables
# (q31 bridge as wasm.cc, the unit's UCXXSRC/UCSRC, and a -I to websim/dsp/legacy
# for the arm_math.h shim) and includes this file.
#
# This is kept as a distinct include from wasm.mk so the root Makefile's project
# discovery recognises gen-1 projects and so any future legacy-only build tweaks
# have a home. The unified `make websim PROJECT=...` command drives it the same
# way (it provides the same `wasm:` target).
##############################################################################

include $(dir $(lastword $(MAKEFILE_LIST)))wasm.mk
