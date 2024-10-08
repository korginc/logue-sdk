PROJECT := $(lastword $(subst /, , $(dir $(abspath $(lastword $(MAKEFILE_LIST))))))
PROJECT_TYPE := $(lastword $(subst -, , $(PROJECT)))
UNIT_NAME := \"$(firstword $(subst -, , $(PROJECT)))\"
UCSRC = header.c
UCXXSRC = unit.cc
UINCDIR  = ../../inc ../common/utils
ULIBS  = -lm
UDEFS = -DUNIT_NAME=$(UNIT_NAME) -DUNIT_TARGET_MODULE=k_unit_module_$(PROJECT_TYPE) -DPARAM_COUNT=10
