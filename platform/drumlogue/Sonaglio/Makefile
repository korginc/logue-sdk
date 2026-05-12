##############################################################################
# Common project definitions
#

MKFILE_PATH := $(realpath $(lastword $(MAKEFILE_LIST)))

# Project root
PROJECT_ROOT ?= $(dir $(MKFILE_PATH))

# Common includes
COMMON_INC_PATH ?= $(realpath $(PROJECT_ROOT)/../common/)

# Common sources
COMMON_SRC_PATH ?= $(realpath $(PROJECT_ROOT)/../common/)

# Installation directory
INSTALLDIR ?= $(PROJECT_ROOT)

##############################################################################
# Include custom project configuration and sources
#

include config.mk

##############################################################################
# Common defaults
#

# Define project name here
PROJECT ?= my_unit

# Target architecture
TARGET_ARCH := armv7a

# Set to 'yes' if you want to see the full log while compiling.
VERBOSE_COMPILE := no

##############################################################################
# Additional source related definitions
#

DINCDIR := $(COMMON_INC_PATH)

CSRC += $(realpath $(COMMON_SRC_PATH)/_unit_base.c)

##############################################################################
# Compiler options
#

# C compiler warnings
USE_CWARN ?= -W -Wall -Wextra

# C++ compiler warnings
USE_CXXWARN ?= -W -Wall -Wextra -Wno-ignored-qualifiers

# Generic compiler options
ifeq ($(USE_OPT),)
  USE_COPT = -pipe
  USE_COPT += -ffast-math
  USE_COPT += -fsigned-char
  USE_COPT += -fno-stack-protector
  USE_COPT += -fstrict-aliasing
  USE_COPT += -falign-functions=16
  USE_COPT += -fno-math-errno
  # USE_COPT += -fpermissive

  USE_CXXOPT = -pipe
  USE_CXXOPT += -ffast-math
  USE_CXXOPT += -fsigned-char
  USE_CXXOPT += -fno-stack-protector
  USE_CXXOPT += -fstrict-aliasing
  USE_CXXOPT += -falign-functions=16
  USE_CXXOPT += -fno-math-errno
  USE_CXXOPT += -fconcepts
  # USE_CXXOPT += -fpermissive
  # USE_CXXOPT += -fmessage-length=0
endif

# C specific options here (added to USE_OPT).
USE_COPT ?=

# C++ specific options here (added to USE_OPT).
USE_CXXOPT ?=

# Debug/Release build dependent options
ifneq ($(DEBUG),)
  USE_OPT += -ggdb3
  USE_LTO := no
  UDEFS += -DDEBUG
  ifeq ($(DEBUG_OPTIM),)
    USE_OPT += -Og ## Debug friendly optimizatiions
  else
    USE_OPT += $(DEBUG_OPTIM)
  endif

  USE_VECTORIZATION := no
else
  # Non-debug
  USE_COPT += -fomit-frame-pointer
  USE_COPT += -finline-limit=9999999999
  USE_COPT += --param max-inline-insns-single=9999999999

  USE_CXXOPT += -fomit-frame-pointer
  USE_CXXOPT += -finline-limit=9999999999
  USE_CXXOPT += --param max-inline-insns-single=9999999999
  USE_CXXOPT += -fno-threadsafe-statics

  ifeq ($(OPTIM),)
    USE_OPT += -Os
  else
    USE_OPT += $(OPTIM)
  endif

  USE_VECTORIZATION := yes
endif 

ifneq ($(NO_INLINE),)
  USE_COPT += -fno-inline
  USE_CXXOPT += -fno-inline
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC := no
endif

# Linker extra options here.
USE_LDOPT ?=

# Enable this if you want link time optimizations (LTO)
USE_LTO ?= yes

##############################################################################
# Compiler settings
#

CC      := $(CROSS_COMPILE)gcc
CXXC    := $(CROSS_COMPILE)g++
LD      := $(CROSS_COMPILE)g++
AS      := $(CROSS_COMPILE)g++
AR      := $(CROSS_COMPILE)ar
CP      := $(CROSS_COMPILE)objcopy
OD      := $(CROSS_COMPILE)objdump
SZ      := $(CROSS_COMPILE)size
STRIP   := $(CROSS_COMPILE)strip
NM      := $(CROSS_COMPILE)nm
RANLIB  := $(CROSS_COMPILE)ranlib
CXXFILT := $(CROSS_COMPILE)c++filt
RM      := rm -f
MV      := mv -f
HEX     := $(CP) -O ihex
BIN     := $(CP) -O binary

# Non-THUMB-specific options here
AOPT :=

# Define C warning options here
CWARN := $(USE_CWARN)

# Define C++ warning options here
CXXWARN := $(USE_CXXWARN)

# Compiler options
OPT    := $(USE_OPT)
COPT   := $(USE_COPT)
CXXOPT := $(USE_CXXOPT)

# Garbage collection
ifeq ($(USE_LINK_GC),yes)
  COPT    += -ffunction-sections -fdata-sections -fno-common
  CXXOPT  += -ffunction-sections -fdata-sections -fno-common
  LDOPT   := --gc-sections
else
  LDOPT :=
endif

# Linker extra options
ifneq ($(USE_LDOPT),)
  LDOPT := $(LDOPT),$(USE_LDOPT)
endif

# Link time optimizations
ifeq ($(USE_LTO),yes)
  OPT += -flto
endif

# CPU/Architecture

ARCH_OPT := -march=armv7-a -mtune=cortex-a7 -marm
OPT += -mfloat-abi=hard -mfpu=neon-vfpv4
ifeq ($(USE_VECTORIZATION),yes)
  OPT += -mvectorize-with-neon-quad
  # OPT += -ffast-math
  OPT += -ftree-vectorize
  OPT += -ftree-vectorizer-verbose=4
  OPT += -funsafe-math-optimizations ## denormals assumed 0, so breaks IEEE754
  DDEF += -D__NEON__
endif
DDEFS += -D__arm__ -D__cortex_a7__

# C Standard
CSTD ?= -std=c11
COPT += $(CSTD)

# C++ Standard
CXXSTD ?= -std=gnu++14
CXXOPT += $(CXXSTD)

# Output directory and files
BUILDDIR ?= build

ifeq ($(BUILDDIR),.)
  BUILDDIR = build
endif

OUTFILES := $(BUILDDIR)/$(PROJECT).drmlgunit \
            $(BUILDDIR)/$(PROJECT).hex \
            $(BUILDDIR)/$(PROJECT).bin \
            $(BUILDDIR)/$(PROJECT).dmp \
            $(BUILDDIR)/$(PROJECT).list

ifdef $(SREC)
  OUTFILES += $(BUILDDIR)/$(PROJECT).srec
endif

# Source files groups and paths
SRC       := $(CSRC) $(CXXSRC)
SRCPATHS  := $(sort $(dir $(ASMXSRC)) $(dir $(ASMSRC)) $(dir $(SRC)))

# Various directories
OBJDIR    := $(BUILDDIR)/obj
LSTDIR    := $(BUILDDIR)/lst
DEPDIR    := .dep

# Object files groups
COBJS     := $(addprefix $(OBJDIR)/, $(notdir $(CSRC:.c=.o)))
CXXOBJS   := $(addprefix $(OBJDIR)/, $(notdir $(CXXSRC:.cc=.o)))
ASMOBJS   := $(addprefix $(OBJDIR)/, $(notdir $(ASMSRC:.s=.o)))
ASMXOBJS  := $(addprefix $(OBJDIR)/, $(notdir $(ASMXSRC:.S=.o)))
OBJS	  := $(ASMXOBJS) $(ASMOBJS) $(COBJS) $(CXXOBJS)

# dependency files
DEPS      := $(addprefix $(DEPDIR)/, $(notdir $(OBJS:%.o=%.o.d)))

# Paths
IINCDIR   := $(patsubst %,-I%,$(INCDIR) $(DINCDIR) $(UINCDIR))
LLIBDIR   := $(patsubst %,-L%,$(DLIBDIR) $(ULIBDIR))

# Macros
DEFS      := $(DDEFS) $(UDEFS)
ADEFS 	  := $(DADEFS) $(UADEFS)

# Libs
LIBS      := $(DLIBS) $(ULIBS)

# Various settings
MCFLAGS   := $(ARCH_OPT)
ODFLAGS	  := -x --syms
ASFLAGS   = $(MCFLAGS) -fPIC -Wa,-amhls=$(LSTDIR)/$(notdir $(<:.s=.lst)) $(ADEFS)
ASXFLAGS  = $(MCFLAGS) -fPIC -Wa,-amhls=$(LSTDIR)/$(notdir $(<:.S=.lst)) $(ADEFS)
CFLAGS    = $(MCFLAGS) $(OPT) $(COPT) $(CWARN) -fPIC -Wa,-alms=$(LSTDIR)/$(notdir $(<:.c=.lst)) $(DEFS)
CXXFLAGS  = $(MCFLAGS) $(OPT) $(CXXOPT) $(CXXWARN) -fPIC -Wa,-alms=$(LSTDIR)/$(notdir $(<:.cc=.lst)) $(DEFS)
LDFLAGS   := $(MCFLAGS) $(OPT) $(LLIBDIR) -shared -Wl,-Map=$(BUILDDIR)/$(PROJECT).map,--cref$(LDOPT)

# Generate dependency information
ASFLAGS  += -MMD -MP -MF .dep/$(@F).d
ASXFLAGS += -MMD -MP -MF .dep/$(@F).d
CFLAGS   += -MMD -MP -MF .dep/$(@F).d
CXXFLAGS += -MMD -MP -MF .dep/$(@F).d

# Paths where to search for sources
VPATH     := $(SRCPATHS)

##############################################################################
# Deduce File Ownership for Build Products
#

USER_ID := $(strip $(shell stat -c %u .))
GROUP_ID := $(strip $(shell stat -c %g .))

##############################################################################
# Rules
#

all: PRE_MAKE_ALL_RULE_HOOK $(OBJS) $(OUTFILES) POST_MAKE_ALL_RULE_HOOK


PRE_MAKE_ALL_RULE_HOOK:

POST_MAKE_ALL_RULE_HOOK: | $(OBJS) $(OUTFILES)
	@chown -R $(USER_ID):$(GROUP_ID) $(BUILDDIR)
	@chown -R $(USER_ID):$(GROUP_ID) .dep

$(OBJS): | $(BUILDDIR) $(OBJDIR) $(LSTDIR)

$(BUILDDIR):
ifneq ($(VERBOSE_COMPILE),yes)
	@echo Compiler Options
	@echo $(CC) -c $(CFLAGS) $(AOPT) -I. $(IINCDIR) xxxx.c -o $(OBJDIR)/xxxx.o
	@echo $(CXXC) -c $(CXXFLAGS) $(AOPT) -I. $(IINCDIR) xxxx.cc -o $(OBJDIR)/xxxx.o
	@echo $(AS) -c $(ASFLAGS) -I. $(IINCDIR) xxxx.s -o $(OBJDIR)/xxxx.o
	@echo $(CC) -c $(ASXFLAGS) -I. $(IINCDIR) xxxx.s -o $(OBJDIR)/xxxx.o
	@echo $(LD) xxxx.o $(LDFLAGS) $(LIBS) -o $(BUILDDIR)/$(PROJECT).elf
	@echo
endif
	@mkdir -p $(BUILDDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(LSTDIR):
	@mkdir -p $(LSTDIR)

$(CXXOBJS) : $(OBJDIR)/%.o : %.cc Makefile
ifeq ($(VERBOSE_COMPILE),yes)
	@echo
	$(CXXC) -c $(CXXFLAGS) $(AOPT) -I. $(IINCDIR) $< -o $@
else
	@echo Compiling $(<F)
	@$(CXXC) -c $(CXXFLAGS) $(AOPT) -I. $(IINCDIR) $< -o $@
endif

$(COBJS) : $(OBJDIR)/%.o : %.c Makefile
ifeq ($(VERBOSE_COMPILE),yes)
	@echo
	$(CC) -c $(CFLAGS) $(AOPT) -I. $(IINCDIR) $< -o $@
else
	@echo Compiling $(<F)
	@$(CC) -c $(CFLAGS) $(AOPT) -I. $(IINCDIR) $< -o $@
endif

$(ASMOBJS) : $(OBJDIR)/%.o : %.s Makefile
ifeq ($(VERBOSE_COMPILE),yes)
	@echo
	$(AS) -c $(ASFLAGS) -I. $(IINCDIR) $< -o $@
else
	@echo Compiling $(<F)
	@$(AS) -c $(ASFLAGS) -I. $(IINCDIR) $< -o $@
endif

$(ASMXOBJS) : $(OBJDIR)/%.o : %.S Makefile
ifeq ($(VERBOSE_COMPILE),yes)
	@echo
	$(CC) -c $(ASXFLAGS) -I. $(IINCDIR) $< -o $@
else
	@echo Compiling $(<F)
	@$(CC) -c $(ASXFLAGS) -I. $(IINCDIR) $< -o $@
endif

$(BUILDDIR)/$(PROJECT).drmlgunit: $(OBJS) #$(LDSCRIPT)
ifeq ($(VERBOSE_COMPILE),yes)
	@echo
	$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@
ifeq ($(DEBUG),)
	$(STRIP) $@
endif
else
	@echo
	@echo Linking $@
	@$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@
ifeq ($(DEBUG),)
	@echo Stripping $@
	@$(STRIP) $@
endif
endif

%.hex: %.drmlgunit
ifeq ($(VERBOSE_COMPILE),yes)
	$(HEX) $< $@
else
	@echo Creating $@
	@$(HEX) $< $@
endif

%.bin: %.drmlgunit
ifeq ($(VERBOSE_COMPILE),yes)
	$(BIN) $< $@
else
	@echo Creating $@
	@$(BIN) $< $@
endif

%.srec: %.drmlgunit
ifdef SREC
  ifeq ($(VERBOSE_COMPILE),yes)
	$(SREC) $< $@
  else
	@echo Creating $@
	@$(SREC) $< $@
  endif
endif

%.dmp: %.drmlgunit
ifeq ($(VERBOSE_COMPILE),yes)
	$(OD) $(ODFLAGS) $< > $@
	$(SZ) $<
else
	@echo Creating $@
	@$(OD) $(ODFLAGS) $< > $@
	@echo
	@$(SZ) $<
endif

%.list: %.drmlgunit
ifeq ($(VERBOSE_COMPILE),yes)
	$(OD) -S $< > $@
else
	@echo Creating $@
	@$(OD) -S $< > $@
	@echo
	@echo Done
	@echo
endif

install: | $(OBJS) $(OUTFILES)
	@echo Deploying to $(INSTALLDIR)/$(PROJECT).drmlgunit
	@mv $(BUILDDIR)/$(PROJECT).drmlgunit $(INSTALLDIR)/
	@echo
	@echo Done
	@echo

clean: CLEAN_RULE_HOOK
	@echo
	@echo Cleaning
	-rm -fR .dep $(BUILDDIR) $(PROJECT_ROOT)/$(PROJECT).drmlgunit
	@echo
	@echo Done
	@echo

CLEAN_RULE_HOOK:

#
# Include the dependency files, should be the last of the makefile
#
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)
