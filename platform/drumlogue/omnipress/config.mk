##############################################################################
# Project Configuration
#

PROJECT := omnipress
PROJECT_TYPE := masterfx

##############################################################################
# Sources
#

# C sources
CSRC = header.c

# C++ sources
CXXSRC = unit.cc

##############################################################################
# Include Paths
#

UINCDIR  = .

##############################################################################
# Compiler Flags
#

# Enable NEON optimizations and aggressive optimization
UCFLAGS = -mfpu=neon -mfloat-abi=softfp -O3 -ftree-vectorize -ffast-math

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

UDEFS = -DARM_NEON_OPTIMIZATION