##############################################################################
# websim shared build fragment
#
# Extracted from the per-project `wasm:` recipe that used to be duplicated
# inline in every NTS-1 mkII / NTS-3 project Makefile. Including this file gives
# a project a `wasm:` target that builds its DSP to WebAssembly with Emscripten
# and launches the browser sandbox. See WEBSIM.md.
#
# A project includes it (after `include config.mk`) like so:
#
#   WEBSIM_SHELL    := osc.html              # osc.html | fx.html | xypad.html | synth.html
#   WEBSIM_INCFLAGS := $(INCDIR)             # -I flags for the unit's own headers
#   include $(SANDBOXDIR)/wasm.mk
#
# Optional overrides (sensible defaults below):
#   WEBSIM_UNITSRC    bridge + unit sources           (default: wasm.cc $(UCSRC) $(UCXXSRC))
#   WEBSIM_DSP        firmware-ROM stand-in sources    (default: websim/dsp/*.c + *.cpp)
#   WEBSIM_EMCC_EXTRA extra emcc flags (e.g. SIMD)     (default: empty)
#   WEBSIM_WORKLET    AudioWorklet processor name      (default: logue-osc)
##############################################################################

# Locate the shared websim assets and the emsdk relative to the project root.
# Every supported project Makefile defines PROJECT_ROOT before including us.
SANDBOXDIR    ?= $(realpath $(PROJECT_ROOT)/../../../websim)
TOOLSDIR      ?= $(realpath $(PROJECT_ROOT)/../../../tools)
EMCC_BIN_PATH ?= $(TOOLSDIR)/emsdk/upstream/emscripten
WASMDIR       ?= $(PROJECT_ROOT)/sim

# Note: do NOT add emsdk's .../emscripten/system/include to the include path.
# Recent Emscripten rejects including headers directly from its source tree
# ("Please use the cache/sysroot/include directory"); emcc resolves its own
# system headers via the sysroot automatically. SIMDe (arm_neon.h) is likewise
# pulled in from the sysroot when building with -msimd128.

# Defaults — a project overrides any of these before `include`-ing this file.
WEBSIM_SHELL    ?= osc.html
WEBSIM_DSP      ?= $(wildcard $(SANDBOXDIR)/dsp/*.c) $(wildcard $(SANDBOXDIR)/dsp/*.cpp)
WEBSIM_UNITSRC  ?= wasm.cc $(UCSRC) $(UCXXSRC)
WEBSIM_INCFLAGS ?=
WEBSIM_EMCC_EXTRA ?=

# Full source list handed to emcc. Note what's intentionally absent vs. the
# hardware build: no _unit_base.c, no CMSIS, no linker script.
WEBSIM_WASMSRC := $(WEBSIM_UNITSRC) $(WEBSIM_DSP)

.PHONY: wasm wasm-build render

# emsdk-bundled node, used to run the offline render harness headlessly.
NODE_BIN ?= $(firstword $(wildcard $(TOOLSDIR)/emsdk/node/*/bin/node))

# Optional: derive a gen-1 unit's legacy param table from its manifest.json
# (instead of a hand-written WEBSIM_LEGACY_PARAM_LIST macro in wasm.cc). A gen-1
# project sets WEBSIM_LEGACY_MANIFEST := <path/to/manifest.json> and includes
# "websim_legacy_params.h" in its wasm.cc. See WEBSIM.md §C.2.
ifdef WEBSIM_LEGACY_MANIFEST
WEBSIM_INCFLAGS   += -I$(WASMDIR)
WEBSIM_GEN_PARAMS := $(WASMDIR)/websim_legacy_params.h
wasm-build render: $(WEBSIM_GEN_PARAMS)
$(WEBSIM_GEN_PARAMS): $(WEBSIM_LEGACY_MANIFEST)
	@mkdir -p $(WASMDIR)
	@python3 $(SANDBOXDIR)/gen_legacy_params.py $< $@
endif

# Build-only: compile the DSP to wasm and stage the page + assets in WASMDIR,
# WITHOUT launching a server. `make wasm` adds the emrun launch on top; the root
# Makefile's `websim-all` reuses this with a per-project WASMDIR to co-serve many
# units from one tree (see WEBSIM.md).
wasm-build:
	@echo Building WebAssembly audio processor
	@mkdir -p $(WASMDIR)
	@cp -r $(SANDBOXDIR)/samples $(WASMDIR)/
	@cp -r $(SANDBOXDIR)/scripts $(WASMDIR)/
	@cp -r $(SANDBOXDIR)/images $(WASMDIR)/
	@cp -r $(WEBSIM_WASMSRC) $(WASMDIR)/
	@$(EMCC_BIN_PATH)/emcc -Wno-unknown-attributes -Wno-limited-postlink-optimizations \
		$(WEBSIM_INCFLAGS) \
		-s AUDIO_WORKLET=1 \
		-s WASM_WORKERS=1 \
		-lembind \
		--shell-file $(SANDBOXDIR)/$(WEBSIM_SHELL) \
		--emrun \
		-O2 \
		-g -fdebug-compilation-dir='..' \
		$(WEBSIM_EMCC_EXTRA) \
		$(WEBSIM_WASMSRC) \
		-o $(WASMDIR)/$(PROJECT).html
	@echo Done

# Build a single unit and launch its sandbox (the standalone flow).
wasm: wasm-build
	@echo Opening the sandbox
	@$(EMCC_BIN_PATH)/emrun --browser chrome --serve_after_close $(WASMDIR)/$(PROJECT).html

# Offline render harness: build the same DSP (same bridge, same SIMD shim) in
# render mode — no AudioWorklet — and run it under node to dump a WAV. Lets audio
# correctness be checked headlessly / in CI without a browser. See WEBSIM.md §B.
#   make render                       -> sim/<project>.render.wav (defaults)
#   make render WEBSIM_RENDER_ARGS="out.wav 69 375"   (osc: out, note, blocks)
#   make render WEBSIM_RENDER_ARGS="out.wav impulse"  (fx: out, sig, blocks, freq)
WEBSIM_RENDER_OUT  ?= $(WASMDIR)/$(PROJECT).render.wav
WEBSIM_RENDER_ARGS ?= $(WEBSIM_RENDER_OUT)
render:
	@echo Building offline render harness
	@mkdir -p $(WASMDIR)
	@$(EMCC_BIN_PATH)/emcc -Wno-unknown-attributes -Wno-limited-postlink-optimizations \
		$(WEBSIM_INCFLAGS) \
		-DWEBSIM_RENDER \
		-sNODERAWFS=1 \
		-sEXIT_RUNTIME=1 \
		-lembind \
		-O2 \
		$(WEBSIM_EMCC_EXTRA) \
		$(WEBSIM_WASMSRC) \
		-o $(WASMDIR)/$(PROJECT).render.js
	@echo Rendering $(PROJECT)
	@$(NODE_BIN) $(WASMDIR)/$(PROJECT).render.js $(WEBSIM_RENDER_ARGS)
