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

# C++ sources
CXXSRC = unit.cc \
		 FmClapModel.cpp \
		 FmCowbellModel.cpp \
		 FmCymbalModel.cpp \
		 FmKickModel.cpp \
		 FmRimshotModel.cpp \
		 FmSnareModel.cpp \
		 FmTomModel.cpp \
		 TRXBassDrum.cpp \
		 TRXClaves.cpp
		 TRXHiHat.cpp \
		 TRXSnareDrum.cpp \

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

