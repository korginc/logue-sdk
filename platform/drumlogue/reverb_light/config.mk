# --- Project Configuration for Light FDN Reverb ---

# 1. Project Name (The filename of your .drmlgunit)
PROJECT := light_fdn_reverb

# 2. Project Type
PROJECT_TYPE := revfx

# 3. C Source Files (Include the metadata header)
CSRC = header.c

# 4. C++ Source Files (The drumlogue SDK bridge)
# Note: unit.cc is the entry point, it includes FDNLightReverb.h
CXXSRC = unit.cc

# List ASM source files here
ASMSRC =

ASMXSRC =

##############################################################################
# Include Paths
#

UINCDIR =
# 5. Optimization & SIMD Flags
# These are added to UDEFS and passed to the main Makefile.
# The main Makefile already adds -mfpu=neon-vfpv4 and -mfloat-abi=hard,
# but we add -O3 and -ffast-math for maximum DSP performance.
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

