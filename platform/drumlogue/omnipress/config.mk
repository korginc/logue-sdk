##############################################################################
# Project Configuration
#

PROJECT := omnipress
PROJECT_TYPE := masterfx

##############################################################################
# Sources
#

CSRC = header.c
CXXSRC = unit.cc

##############################################################################
# Include Paths
#

UINCDIR  = .

##############################################################################
# Compiler Flags
#

UCFLAGS = -mfpu=neon -mfloat-abi=softfp -O3 -ftree-vectorize -ffast-math

##############################################################################
# Libraries
#

ULIBS  = -lm
ULIBS += -lc

##############################################################################
# Macros
#

UDEFS = -DARM_NEON_OPTIMIZATION