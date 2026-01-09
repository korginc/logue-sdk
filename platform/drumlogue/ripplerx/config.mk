##############################################################################
# Project Configuration
#

PROJECT := resonator
PROJECT_TYPE := synth

##############################################################################
# Sources
#

# C sources
CSRC = header.c

# C++ sources
CXXSRC = unit.cc ripplerx.cc Envelope.cc Filter.cc Mallet.cc Resonator.cc Partial.cc Waveguide.cc Noise.cc Voice.cc Models.cc Sampler.cc

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

