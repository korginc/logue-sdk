# #############################################################################
# Oscillator WASM Makefile
# #############################################################################
# usage: emmake make -f WASM.mak
# or via main Makefile wasm target

BUILD_TARGET = TARGET_WASM

PLATFORMDIR = ..
PROJECTDIR = .
TOOLSDIR = $(PLATFORMDIR)/../../tools
EXTDIR = $(PLATFORMDIR)/../ext

WAB = $(EXTDIR)/WAB
WAB_CPP = $(WAB)/cpp
WAB_JS = $(WAB)/js

MANIFEST = $(PROJECTDIR)/manifest.json

## Project Sources
include $(PROJECTDIR)/project.mk

# #############################################################################
# configure compilation
# #############################################################################

COPT = -std=c11 
CXXOPT = -std=c++11 -fno-exceptions -fno-non-call-exceptions

LDOPT =

EMFLAGS =  -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s BINARYEN_ASYNC_COMPILATION=0 --bind
EMFLAGS += -s EXPORT_NAME="'WABModule'" -s SINGLE_FILE=1

DDEFS = -D$(BUILD_TARGET) -DFORCE_FTZ

OPT =  -O2
CWARN = -W -Wall -Wextra -Wno-unknown-attributes
CXXWARN = -Wall -Wno-unknown-attributes

# #############################################################################
# set targets and directories
# #############################################################################

PKGDIR = $(PROJECT)
MANIFEST = $(PROJECTDIR)/manifest.json
PAYLOAD = $(PROJECT).js
BUILDDIR = $(PROJECTDIR)/build
OBJDIR = $(BUILDDIR)/emcc_obj
LSTDIR = $(BUILDDIR)/emcc_lst

CSRC = $(UCSRC)
CXXSRC = $(UCXXSRC) \
         $(PROJECTDIR)/tpl/_wasm_unit.cpp \
         $(WAB_CPP)/logue_wrapper.cpp \
         $(WAB_CPP)/wab_processor.cpp

vpath %.c $(sort $(dir $(CSRC)))
vpath %.cpp $(sort $(dir $(CXXSRC)))

COBJS := $(addprefix $(OBJDIR)/, $(notdir $(CSRC:.c=.o)))
CXXOBJS := $(addprefix $(OBJDIR)/, $(notdir $(CXXSRC:.cpp=.o)))
JSOBJS := pre.js

OBJS := $(COBJS) $(CXXOBJS) $(JSOBJS)

DINCDIR = $(PROJECTDIR)/inc \
	  $(PROJECTDIR)/inc/api \
          $(PLATFORMDIR)/inc \
	  $(PLATFORMDIR)/inc/dsp \
	  $(PLATFORMDIR)/inc/utils \
	  $(WAB_CPP)

INCDIR := $(patsubst %,-I%,$(DINCDIR) $(UINCDIR))

DEFS := $(DDEFS) $(UDEFS)

DLIBDIR = $(PROJECTDIR)/ld

LIBS = $(DLIBS) $(ULIBS) $(PROJECTDIR)/ld/wasm_osc_api.a

LIBDIR := $(patsubst %,-I%,$(DLIBDIR) $(ULIBDIR))

# #############################################################################
# compiler flags
# #############################################################################

CFLAGS    = $(OPT) $(COPT) $(CWARN) $(EMFLAGS) $(DEFS)
CXXFLAGS  = $(OPT) $(CXXOPT) $(CXXWARN) $(EMFLAGS) $(DEFS)
LDFLAGS   = $(OPT) $(LIBDIR) $(LDOPT) $(EMFLAGS) --pre-js $(OBJDIR)/pre.js

OUTFILES := $(BUILDDIR)/$(PAYLOAD)

###############################################################################
# targets
###############################################################################

all: PRE_ALL $(OBJS) $(OUTFILES) POST_ALL

PRE_ALL:

POST_ALL: 

$(OBJS): | $(BUILDDIR) $(OBJDIR) $(LSTDIR)

$(BUILDDIR):
	@echo "- Compiler Options:"
	@echo $(CC) -c $(CFLAGS) -I. $(INCDIR)
	@echo $(CC) -c $(CXXFLAGS) -I. $(INCDIR)
	@echo
	@mkdir -p $(BUILDDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(LSTDIR):
	@mkdir -p $(LSTDIR)

$(JSOBJS) : $(MANIFEST)
	@echo "- Processing $(<F)"
	@echo 'WABModule.manifest = ' > $(OBJDIR)/pre.js
	@cat $(MANIFEST) >> $(OBJDIR)/pre.js

$(COBJS) : $(OBJDIR)/%.o : %.c WASM.mak
	@echo "- Compiling $(<F)"
	@$(CC) -c $(CFLAGS) -I. $(INCDIR) $< -o $@

$(CXXOBJS) : $(OBJDIR)/%.o : %.cpp WASM.mak
	@echo "- Compiling $(<F)"
	@$(CC) -c $(CXXFLAGS) -I. $(INCDIR) $< -o $@

$(BUILDDIR)/$(PAYLOAD): $(OBJS)
	@echo
	@echo "- Generating $@"
	-$(LD) $(COBJS) $(CXXOBJS) $(LDFLAGS) $(LIBS) -o $@ 

clean:
	@echo "Cleaning Web Assembly products"
	-rm -fR .dep $(OBJDIR) $(LSTDIR) $(OUTFILES)
	@echo
