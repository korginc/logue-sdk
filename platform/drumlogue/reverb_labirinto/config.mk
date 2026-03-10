##############################################################################
# Configuration for Makefile
#

PROJECT := neon_labirinto_reverb
PROJECT_TYPE := revfx

##############################################################################
# Sources
#

# C sources
CSRC = header.c

# C++ sources
CXXSRC = unit.cc

# List ASM source files here
ASMSRC =

ASMXSRC =

##############################################################################
# Include Paths
#

UINCDIR =
# --- Compiler Flags (Il cuore dell'ottimizzazione NEON) ---
# -mfpu=neon: Attiva l'unità SIMD NEON
# -mfloat-abi=hard: Usa i registri hardware per il passaggio dei float (fondamentale per performance)
# -O3: Massima ottimizzazione del codice
# -ffast-math: Permette ottimizzazioni matematiche aggressive (sicuro per questo DSP)
UDEFS = -mfpu=neon -mfloat-abi=hard -O3 -ffast-math

##############################################################################
# Library Paths
#

ULIBDIR =

##############################################################################
# Libraries
#

ULIBS  = -lm
ULIBS += -lc

##############################################################################
# Macros
#

UDEFS =

