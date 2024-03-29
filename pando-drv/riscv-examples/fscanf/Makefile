# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
RISCV_CSOURCE := $(wildcard *.c)
THREADS ?= 8
SIM_OPTIONS += --verbose 0
SIM_OPTIONS += --core-threads=$(THREADS) --pod-cores=1

#RISCV_COMPILE_FLAGS += -DLOCK
RISCV_COMPILE_FLAGS += -DTHREADS=$(THREADS) -DBARRIER
RISCV_PLATFORM_LIBC_LOCKING := yes
include $(DRV_DIR)/mk/riscv_common.mk
