##############################################################################
# Project Configuration
#

PROJECT := scruta_astri
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