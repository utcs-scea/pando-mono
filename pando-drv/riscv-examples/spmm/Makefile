# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

DRV_DIR := $(shell git rev-parse --show-toplevel)
TARGET := exe.riscv
RISCV_CXXSOURCE := $(wildcard *.cpp)
#SIM_OPTIONS += --verbose 1000
#SIM_OPTIONS += --verbose 1000 --debug-memory --debug-responses
SIM_OPTIONS += --debug-syscalls --core-threads=1 --pod-cores=1
include $(DRV_DIR)/mk/riscv_common.mk
