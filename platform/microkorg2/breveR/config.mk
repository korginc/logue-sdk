##############################################################################
# Project Configuration
#

PROJECT := breveR
PROJECT_TYPE := revfx

##############################################################################
# Sources
#

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = header.c

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CXXSRC = unit.cc

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC = 

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACXXSRC = 

# List ASM source files here
ASMSRC = 

ASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = 

ifeq ($(ARCH), arm)
  UINCDIR += 
else
  UINCDIR += 
endif

##############################################################################
# Library Paths
#

ULIBDIR = 

ifeq ($(ARCH), arm)
  ULIBDIR += 
else
  ULIBDIR += 
endif

##############################################################################
# Libraries
#

ULIBS  = -lm
ULIBS += -lc

##############################################################################
# Macros
#

UDEFS = 

