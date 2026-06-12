##############################################################################
# Project Configuration
#

PROJECT := effemd
PROJECT_TYPE := synth

##############################################################################
# Sources
#

# C sources
CSRC = header.c

# C++ sources (the SDK Makefile only builds .cc files)
CXXSRC = unit.cc \
		 FmClapModel.cc \
		 FmCowbellModel.cc \
		 FmCymbalModel.cc \
		 FmKickModel.cc \
		 FmRimshotModel.cc \
		 FmSnareModel.cc \
		 FmTomModel.cc \
		 TRXBassDrum.cc \
		 TRXClaves.cc \
		 TRXHiHat.cc \
		 TRXSnareDrum.cc

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

