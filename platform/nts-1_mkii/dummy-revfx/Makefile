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

# Tools directory
TOOLSDIR ?= $(PROJECT_ROOT)/../../../tools

# External library directory
EXTDIR ?= $(PROJECT_ROOT)/../../ext

# CMSIS library location
CMSISDIR ?= $(EXTDIR)/CMSIS/CMSIS

# Linker scripts location
LDDIR ?= $(PROJECT_ROOT)/../ld

##############################################################################
# Include custom project configuration and sources
#

include config.mk

##############################################################################
# Common defaults
#

# Define project name here
PROJECT ?= my_unit

##############################################################################
# Setup cross compilation
#

MCU := cortex-m7

MCU_MODEL := STM32H725xE

GCC_TARGET := arm-none-eabi-
GCC_BIN_PATH ?= $(TOOLSDIR)/gcc/gcc-arm-none-eabi-10.3-2021.10/bin

CC    := $(GCC_BIN_PATH)/$(GCC_TARGET)gcc
CXXC  := $(GCC_BIN_PATH)/$(GCC_TARGET)g++
LD    := $(GCC_BIN_PATH)/$(GCC_TARGET)gcc
#LD   := $(GCC_BIN_PATH)/$(GCC_TARGET)g++
CP    := $(GCC_BIN_PATH)/$(GCC_TARGET)objcopy
AS    := $(GCC_BIN_PATH)/$(GCC_TARGET)gcc -x assembler-with-cpp
AR    := $(GCC_BIN_PATH)/$(GCC_TARGET)ar
OD    := $(GCC_BIN_PATH)/$(GCC_TARGET)objdump
SZ    := $(GCC_BIN_PATH)/$(GCC_TARGET)size
STRIP := $(GCC_BIN_PATH)/$(GCC_TARGET)strip

HEX   := $(CP) -O ihex
BIN   := $(CP) -O binary

RULESPATH := $(LDDIR)
LDSCRIPT := $(LDDIR)/unit.ld
DLIBS := -lc

DADEFS := -D$(MCU_MODEL) -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM7
DDEFS := -D$(MCU_MODEL) -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM7 -D__FPU_PRESENT

COPT := -fPIC -std=c11 -fno-exceptions
CXXOPT := -fPIC -fno-use-cxa-atexit -std=c++11 -fno-rtti -fno-exceptions -fno-non-call-exceptions

LDOPT := -shared --entry=0 -specs=nano.specs -specs=nosys.specs

CWARN := -W -Wall -Wextra
CXXWARN :=

FPU_OPTS := -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new

OPT := -g -Os -mlittle-endian 
OPT += $(FPU_OPTS)

## TODO: there seems to be a bug or some yet unknown behavior that breaks PLT code for external calls when LTO is enabled alongside -nostartfiles
#OPT += -flto

TOPT := -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT

##############################################################################
# Set compilation targets and directories
#

PRODUCT := $(PROJECT).nts1mkiiunit

BUILDDIR := $(PROJECT_ROOT)/build
OBJDIR := $(BUILDDIR)/obj
LSTDIR := $(BUILDDIR)/lst

ASMSRC := $(UASMSRC)

ASMXSRC := $(UASMXSRC)

CSRC := $(UCSRC)
CSRC += $(realpath $(COMMON_SRC_PATH)/_unit_base.c)

CXXSRC := $(UCXXSRC)

vpath %.s $(sort $(dir $(ASMSRC)))
vpath %.S $(sort $(dir $(ASMXSRC)))
vpath %.c $(sort $(dir $(CSRC)))
vpath %.cc $(sort $(dir $(CXXSRC)))

ASMOBJS := $(addprefix $(OBJDIR)/, $(notdir $(ASMSRC:.s=.o)))
ASMXOBJS := $(addprefix $(OBJDIR)/, $(notdir $(ASMXSRC:.S=.o)))
COBJS := $(addprefix $(OBJDIR)/, $(notdir $(CSRC:.c=.o)))
CXXOBJS := $(addprefix $(OBJDIR)/, $(notdir $(CXXSRC:.cc=.o)))

OBJS := $(ASMXOBJS) $(ASMOBJS) $(COBJS) $(CXXOBJS)

DINCDIR := $(COMMON_INC_PATH) \
           $(CMSISDIR)/Include

INCDIR := $(patsubst %,-I%,$(DINCDIR) $(UINCDIR))

DEFS := $(DDEFS) $(UDEFS)
ADEFS := $(DADEFS) $(UADEFS)

LIBS := $(DLIBS) $(ULIBS)

LIBDIR := $(patsubst %,-I%,$(DLIBDIR) $(ULIBDIR))

##############################################################################
# Compiler flags
#

MCFLAGS   := -mcpu=$(MCU)
ODFLAGS	  := -x --syms
ASFLAGS   = $(MCFLAGS) -g $(TOPT) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.s=.lst)) $(ADEFS)
ASXFLAGS  = $(MCFLAGS) -g $(TOPT) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.S=.lst)) $(ADEFS)
CFLAGS    = $(MCFLAGS) $(TOPT) $(OPT) $(COPT) $(CWARN) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.c=.lst)) $(DEFS)
CXXFLAGS  = $(MCFLAGS) $(TOPT) $(OPT) $(CXXOPT) $(CXXWARN) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.cc=.lst)) $(DEFS)
LDFLAGS   := $(MCFLAGS) $(TOPT) $(OPT) -nostartfiles $(LIBDIR) -Wl,-z,max-page-size=128,-Map=$(BUILDDIR)/$(PROJECT).map,--cref,--no-warn-mismatch,--library-path=$(RULESPATH),--script=$(LDSCRIPT) $(LDOPT)

OUTFILES := $(BUILDDIR)/$(PROJECT).elf \
	    $(BUILDDIR)/$(PROJECT).hex \
	    $(BUILDDIR)/$(PROJECT).bin \
	    $(BUILDDIR)/$(PROJECT).dmp \
	    $(BUILDDIR)/$(PROJECT).list

##############################################################################
# Targets
#

all: PRE_ALL $(OBJS) $(OUTFILES) POST_ALL
	@echo Done
	@echo

PRE_ALL:

POST_ALL:

$(OBJS): | $(BUILDDIR) $(OBJDIR) $(LSTDIR)

$(BUILDDIR):
	@echo Compiler Options
	@echo $(CC) -c $(CFLAGS) -I. $(INCDIR)
	@echo
	@mkdir -p $(BUILDDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(LSTDIR):
	@mkdir -p $(LSTDIR)

$(ASMOBJS) : $(OBJDIR)/%.o : %.s Makefile
	@echo Assembling $(<F)
	@$(AS) -c $(ASFLAGS) -I. $(INCDIR) $< -o $@

$(ASMXOBJS) : $(OBJDIR)/%.o : %.S Makefile
	@echo Assembling $(<F)
	@$(CC) -c $(ASXFLAGS) -I. $(INCDIR) $< -o $@

$(COBJS) : $(OBJDIR)/%.o : %.c Makefile
	@echo Compiling $(<F)
	@$(CC) -c $(CFLAGS) -I. $(INCDIR) $< -o $@

$(CXXOBJS) : $(OBJDIR)/%.o : %.cc Makefile
	@echo Compiling $(<F)
	@$(CXXC) -c $(CXXFLAGS) -I. $(INCDIR) $< -o $@

$(BUILDDIR)/%.elf: $(OBJS) $(LDSCRIPT)
	@echo Linking $@
	@echo $(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@
	@$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%.hex: %.elf
	@echo Creating $@
	@$(HEX) $< $@

%.bin: %.elf
	@echo Creating $@
	@$(BIN) $< $@

%.dmp: %.elf
	@echo Creating $@
	@$(OD) $(ODFLAGS) $< > $@
	@echo
	@$(SZ) $<
	@echo

%.list: %.elf
	@echo Creating $@
	@$(OD) -S $< > $@

clean:
	@echo Cleaning
	-rm -fR $(PROJECT_ROOT)/.dep $(BUILDDIR) $(PROJECT_ROOT)/$(PRODUCT)
	@echo Done
	@echo

$(BUILDDIR)/$(PRODUCT): | $(OBJS) $(OUTFILES)
	@echo Making $(BUILDDIR)/$(PRODUCT)
	@cp -a $(BUILDDIR)/$(PROJECT).elf $(BUILDDIR)/$(PRODUCT)
	@$(STRIP) $(BUILDDIR)/$(PRODUCT)

install: $(BUILDDIR)/$(PRODUCT)
	@echo Deploying to $(INSTALLDIR)/$(PRODUCT)
	@mv $(BUILDDIR)/$(PRODUCT) $(INSTALLDIR)/$(PRODUCT)
	@echo Done
	@echo

