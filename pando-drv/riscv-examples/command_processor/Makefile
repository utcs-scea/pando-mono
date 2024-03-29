# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)

# pando hammer source files
RISCV_CSOURCE += hello.c

# command processor source files
COMMAND_PROCESSOR_CXXSOURCE += hello_cp.cpp
COMMAND_PROCESSOR_PLATFORM_LOADER := no

SIM_OPTIONS += --core-threads=1 --pod-cores=1

include $(DRV_DIR)/mk/riscv_common.mk
include $(DRV_DIR)/mk/command_processor.mk
