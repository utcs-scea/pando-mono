# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)

DRAM_BASE := 0x800000000000
DRAM_BASE_D := $(shell printf "%d" $(DRAM_BASE))

L2SP_BASE := 0x000102000000
L2SP_BASE_D := $(shell printf "%d" $(L2SP_BASE))

TEST      := 0x000102100d60
TEST_D    := $(shell printf "%d" $(TEST))

COMPILE_FLAGS += -DCP_TO_PH_ADDR=$(shell echo $(TEST_D) +  0 | bc)
COMPILE_FLAGS += -DPH_TO_CP_ADDR=$(shell echo $(TEST_D) + 16 | bc)

# pando hammer source files
RISCV_COMPILE_FLAGS += $(COMPILE_FLAGS)
RISCV_CSOURCE += ph.c

# command processor source files
COMMAND_PROCESSOR_COMPILE_FLAGS += $(COMPILE_FLAGS)
COMMAND_PROCESSOR_CXXSOURCE += cp.cpp
COMMAND_PROCESSOR_PLATFORM_LOADER := no

SIM_OPTIONS += --core-threads=1 --pod-cores=1

include $(DRV_DIR)/mk/riscv_common.mk
include $(DRV_DIR)/mk/command_processor.mk
