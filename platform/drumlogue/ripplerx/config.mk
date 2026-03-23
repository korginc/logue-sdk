##############################################################################
# Project Configuration
#

PROJECT := ripplerx
PROJECT_TYPE := synth

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

UINCDIR  =

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
# RENDER_STAGE: Incremental debug build — uncomment ONE line to isolate silence:
#   Stage 1 — Raw exciter only (mallet impulse / PCM sample, no waveguide)
#   Stage 2 — + Waveguide resonators
#   Stage 3 — + master_env fade + voice squelch
#   Stage 4 — + Tone EQ + master filter + overdrive  (default when unset)
# UDEFS += -DRENDER_STAGE=1

