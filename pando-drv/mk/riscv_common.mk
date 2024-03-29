# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

ifndef _RISCV_COMMON_MK_
_RISCV_COMMON_MK_ := 1

DRV_DIR ?= $(shell git rev-parse --show-toplevel)
export PYTHONPATH := $(DRV_DIR)/tests:$(PYTHONPATH)

include $(DRV_DIR)/mk/config.mk

RISCV_CXX := $(RISCV_INSTALL_DIR)/bin/riscv64-$(RISCV_ARCH)-g++
RISCV_CC  := $(RISCV_INSTALL_DIR)/bin/riscv64-$(RISCV_ARCH)-gcc
RISCV_OBJDUMP := $(RISCV_INSTALL_DIR)/bin/riscv64-$(RISCV_ARCH)-objdump

RISCV_PLATFORM ?= ph

vpath %.S $(DRV_DIR)/riscv-examples/platform_$(RISCV_PLATFORM)
vpath %.c $(DRV_DIR)/riscv-examples/platform_$(RISCV_PLATFORM)
vpath %.cpp $(DRV_DIR)/riscv-examples/platform_$(RISCV_PLATFORM)

RISCV_ARCH:=rv64imafd
RISCV_ABI:=lp64d

RISCV_COMPILE_FLAGS += -O2 -march=$(RISCV_ARCH)
RISCV_COMPILE_FLAGS += -mabi=$(RISCV_ABI)

RISCV_CXXFLAGS += $(RISCV_COMPILE_FLAGS)
RISCV_CFLAGS   += $(RISCV_COMPILE_FLAGS)
RISCV_LDFLAGS  +=
RISCV_LIBS     +=

-include $(DRV_DIR)/riscv-examples/platform_$(RISCV_PLATFORM)/common.mk

# The application defines this to add C source files
# to RISCV executable
RISCV_CSOURCE   ?=
RISCV_COBJECT   += $(patsubst %.c,%.o,$(RISCV_CSOURCE))

# The application defines this to add C++ [.cpp] source files
# to RISCV executable
RISCV_CXXSOURCE ?=
RISCV_CXXOBJECT := $(patsubst %.cpp,%.o,$(RISCV_CXXSOURCE))

# The application defines this to add .S source files
# to RISCV executable
RISCV_ASMSOURCE ?=
RISCV_ASMOBJECT := $(patsubst %.S,%.o,$(RISCV_ASMSOURCE))

$(RISCV_ASMOBJECT): %.o: %.S
  $(RISCV_CC) $(RISCV_CFLAGS) -c -o $@ $<

$(RISCV_COBJECT): %.o: %.c
  $(RISCV_CC) $(RISCV_CFLAGS) -c -o $@ $<

$(RISCV_CXXOBJECT): %.o: %.cpp
  $(RISCV_CXX) $(RISCV_CXXFLAGS) -c -o $@ $<

RISCV_TARGET ?= exe.riscv

$(RISCV_TARGET): %.riscv: $(RISCV_COBJECT) $(RISCV_CXXOBJECT) $(RISCV_ASMOBJECT)
  $(RISCV_CXX) $(RISCV_CXXFLAGS) $(RISCV_LDFLAGS) -o $@ $(filter %.o,$^) $(RISCV_LIBS)

.PHONY: clean
clean:
  rm -f *.o *.riscv

SIM_OPTIONS ?=

run: $(RISCV_TARGET)
  $(SST) $(SCRIPT) -- $(SIM_OPTIONS) $(RISCV_TARGET) $(SIM_ARGS)

disassemble: $(RISCV_TARGET)
  $(RISCV_OBJDUMP) -D $(RISCV_TARGET)

opcode-mix: $(RISCV_TARGET)
  python3 $(DRV_DIR)/py/opcode-mix.py stats.csv tags.csv

include $(DRV_DIR)/mk/command_processor.mk

endif
