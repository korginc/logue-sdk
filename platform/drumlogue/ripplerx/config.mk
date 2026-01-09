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
CXXSRC = unit.cc ripplerx.cpp Envelope.cpp Filter.cpp Mallet.cpp Resonator.cpp Partial.cpp Waveguide.cpp Noise.cpp Voice.cpp Models.cpp Sampler.cpp

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

