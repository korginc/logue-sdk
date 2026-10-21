##############################################################################
# logue-sdk root Makefile — build & launch the web simulator (websim)
#
# Convenience wrapper around the per-project `make wasm` flow. See WEBSIM.md.
#
# Quick start:
#   make setup                              # one-time: fetch + install emsdk
#   make list                               # show wasm-capable projects
#   make websim                             # launch the default project (waves)
#   make websim PROJECT=platform/nts-3_kaoss/pluck
#   make websim PROJECT=dummy-osc           # bare name is resolved if unambiguous
#   make clean   PROJECT=waves
#
# Requirements: git, python3, GNU make, and Google Chrome (the per-project wasm
# target launches `emrun --browser chrome`). On Windows use Git Bash, MSYS2, or WSL.
##############################################################################

# Default project to launch (path relative to repo root, or a bare project name).
PROJECT ?= platform/nts-1_mkii/waves

EMSDK_DIR     := tools/emsdk
EMSDK         := $(EMSDK_DIR)/emsdk
EMCC_BIN_PATH := $(EMSDK_DIR)/upstream/emscripten

# Auto-discover every project that can build a wasm sandbox: either it defines a
# literal `wasm:` target (legacy/inline) or it includes a shared websim fragment
# (websim/wasm.mk for gen-2, websim/legacy.mk for gen-1). Matching both keeps any
# not-yet-migrated project working during the transition.
WASM_PROJECTS := $(patsubst %/Makefile,%,$(shell \
    grep -rlE '^(wasm:|[[:space:]]*include .*/(wasm|legacy)\.mk)' \
    platform/*/*/Makefile 2>/dev/null))

# Projects that support the headless offline render harness (`make render`):
# those whose wasm.cc uses a shared bridge with a WEBSIM_RENDER path. Excludes
# the inline nts-1_mkii / nts-3 bridges (no render main yet).
RENDER_PROJECTS := $(patsubst %/wasm.cc,%,$(shell \
    grep -rlE 'mk2_osc_bridge\.h|mk2_fx_bridge\.h|dl_synth_bridge\.h|dl_fx_bridge\.h|legacy_osc_bridge\.h|legacy_fx_bridge\.h' \
    platform/*/*/wasm.cc 2>/dev/null))

# Resolve $(PROJECT) to a project directory at recipe time:
#   - a real path containing a Makefile is used as-is
#   - otherwise it's matched as a bare name against the discovered wasm projects
#   - 0 matches -> error with the list; >1 matches -> error asking for the full path
define resolve_project
	dir=""; \
	if [ -f "$(PROJECT)/Makefile" ]; then \
	  dir="$(PROJECT)"; \
	else \
	  matches=$$(printf '%s\n' $(WASM_PROJECTS) | grep -E "/$(PROJECT)$$" || true); \
	  count=$$(printf '%s\n' $$matches | grep -c . || true); \
	  if [ "$$count" = "1" ]; then \
	    dir="$$matches"; \
	  elif [ "$$count" = "0" ]; then \
	    echo "error: no wasm-capable project matches '$(PROJECT)'."; \
	    echo "Run 'make list' to see the available projects."; \
	    exit 1; \
	  else \
	    echo "error: '$(PROJECT)' is ambiguous; matches:"; \
	    printf '  %s\n' $$matches; \
	    echo "Re-run with the full path, e.g. PROJECT=platform/nts-1_mkii/$(PROJECT)"; \
	    exit 1; \
	  fi; \
	fi
endef

.DEFAULT_GOAL := help

.PHONY: help setup check list websim run clean render render-all build-all

help:
	@echo "logue-sdk websim launcher"
	@echo ""
	@echo "Targets:"
	@echo "  setup            One-time: fetch the emsdk submodule and install/activate Emscripten."
	@echo "  list             List all wasm-capable projects."
	@echo "  websim           Build a single project to WebAssembly and open it in the browser."
	@echo "  render           Headlessly render one PROJECT to a WAV (no browser); see WEBSIM.md B."
	@echo "  render-all       Render every render-capable project and assert finite output (CI smoke)."
	@echo "  build-all        Compile every wasm-capable project (build only); fail on any error (CI)."
	@echo "  clean            Remove a project's build/ and sim/ output."
	@echo "  help             Show this message."
	@echo ""
	@echo "Variables:"
	@echo "  PROJECT          Project path or bare name for 'websim' (default: $(PROJECT))."
	@echo ""
	@echo "Examples:"
	@echo "  make setup"
	@echo "  make websim                                            # default (NTS-1 mkII waves)"
	@echo "  make websim PROJECT=platform/nts-3_kaoss/pluck"
	@echo "  make websim PROJECT=platform/microkorg2/waves          # microKORG2 (gen-2)"
	@echo "  make websim PROJECT=platform/drumlogue/dummy-synth     # drumlogue (stereo synth)"
	@echo "  make websim PROJECT=platform/nutekt-digital/waves      # NTS-1 mkI / gen-1 osc"
	@echo "  make websim PROJECT=platform/minilogue-xd/waves        # minilogue xd / gen-1 osc"
	@echo "  make websim PROJECT=platform/prologue/waves            # prologue / gen-1 osc"
	@echo "  make websim PROJECT=dummy-osc                          # bare name (must be unambiguous)"

# Clear stale emsdk env vars for the bootstrap. Sourcing emsdk_env.sh in a
# shell profile exports EMSDK_PYTHON / EMSDK_NODE / SSL_CERT_FILE pointing into
# bundled tool dirs (python, node, certifi) that a fresh checkout hasn't
# downloaded yet. They then break the very `emsdk install` meant to fetch them
# (missing python interpreter, missing CA bundle for curl). Unset them so the
# wrapper falls back to python3 / the system CA store from PATH.
BOOTSTRAP_ENV := env -u EMSDK_PYTHON -u EMSDK_NODE -u SSL_CERT_FILE -u CURL_CA_BUNDLE -u REQUESTS_CA_BUNDLE

setup:
	@echo "==> Fetching emsdk submodule"
	git submodule update --init $(EMSDK_DIR)
	@echo "==> Installing latest Emscripten (this may take a while)"
	cd $(EMSDK_DIR) && $(BOOTSTRAP_ENV) ./emsdk install latest
	@echo "==> Activating latest Emscripten"
	cd $(EMSDK_DIR) && $(BOOTSTRAP_ENV) ./emsdk activate latest
	@echo "==> Done. You can now run 'make websim'."

check:
	@test -x $(EMSDK) || { \
	  echo "error: Emscripten not set up ($(EMSDK) missing)."; \
	  echo "Run 'make setup' first."; \
	  exit 1; \
	}
	@test -x $(EMSDK_DIR)/upstream/emscripten/emcc || { \
	  echo "error: emcc not found — emsdk is fetched but not installed/activated."; \
	  echo "Run 'make setup' (or: cd $(EMSDK_DIR) && ./emsdk install latest && ./emsdk activate latest)."; \
	  exit 1; \
	}

list:
	@echo "wasm-capable projects:"
	@printf '  %s\n' $(WASM_PROJECTS)

# Build one project to WebAssembly and launch its browser sandbox via emrun.
websim run: check
	@$(resolve_project); \
	echo "==> Launching websim for $$dir"; \
	$(MAKE) -C "$$dir" wasm

# Headless offline render: build a single project in render mode and dump a WAV
# (no browser). See WEBSIM.md §B. Forward args via WEBSIM_RENDER_ARGS.
#   make render PROJECT=platform/microkorg2/vox WEBSIM_RENDER_ARGS="out.wav 69"
render: check
	@$(resolve_project); \
	echo "==> Rendering $$dir"; \
	$(MAKE) -C "$$dir" render WEBSIM_RENDER_ARGS="$(WEBSIM_RENDER_ARGS)"

# CI smoke: render every render-capable project headlessly and assert the output
# is finite (no NaN/Inf). Silent templates pass; this catches crashes and
# numerical blow-ups without a browser. See WEBSIM.md §B and the CI notes (§E).
render-all: check
	@fail=""; for p in $(RENDER_PROJECTS); do \
	  plat=$$(basename $$(dirname $$p)); proj=$$(basename $$p); \
	  echo "==> render $$plat/$$proj"; \
	  if ! $(MAKE) --no-print-directory -C "$$p" render >/dev/null 2>&1; then \
	    echo "!!  $$plat/$$proj: render failed"; fail="$$fail $$plat/$$proj"; continue; \
	  fi; \
	  wav=$$(ls "$$p"/sim/*.render.wav 2>/dev/null | head -1); \
	  if [ -z "$$wav" ] || ! python3 websim/scripts/check_render.py "$$wav" >/dev/null 2>&1; then \
	    echo "!!  $$plat/$$proj: check failed"; fail="$$fail $$plat/$$proj"; \
	  else echo "    ok ($$wav)"; fi; \
	done; \
	if [ -n "$$fail" ]; then printf '\nrender-all FAILED:%s\n' "$$fail"; exit 1; fi; \
	echo "render-all: all renders finite and OK"

# CI build smoke: compile every wasm-capable project (build only, no browser) and
# fail on the first compile/link error. See WEBSIM.md (§E).
build-all: check
	@fail=""; for p in $(WASM_PROJECTS); do \
	  plat=$$(basename $$(dirname $$p)); proj=$$(basename $$p); \
	  echo "==> build $$plat/$$proj"; \
	  if ! $(MAKE) --no-print-directory -C "$$p" wasm-build >/dev/null 2>&1; then \
	    echo "!!  $$plat/$$proj: build FAILED"; fail="$$fail $$plat/$$proj"; \
	  fi; \
	done; \
	if [ -n "$$fail" ]; then printf '\nbuild-all FAILED:%s\n' "$$fail"; exit 1; fi; \
	echo "build-all: $(words $(WASM_PROJECTS)) project(s) built OK"

clean:
	@$(resolve_project); \
	echo "==> Cleaning $$dir"; \
	$(MAKE) -C "$$dir" clean
