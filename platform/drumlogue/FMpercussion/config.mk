##############################################################################
# Project Configuration
#

PROJECT := fm_perc_synth
PROJECT_TYPE := synth

##############################################################################
# Sources
#

# C sources
CSRC = header.c

# C++ sources
CXXSRC = unit.cc

# List all your FM percussion synth source files
# Note: These are all headers, so no need to list them in sources
# They will be included via synth.h

##############################################################################
# Include Paths
#

UINCDIR  = . 

##############################################################################
# Compiler Flags
#

# Enable NEON optimizations
UCFLAGS = -mfpu=neon -mfloat-abi=softfp -O3 -ftree-vectorize

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