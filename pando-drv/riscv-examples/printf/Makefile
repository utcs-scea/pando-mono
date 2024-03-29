# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
RISCV_CSOURCE := $(wildcard *.c)
THREADS := 4
CORES := 2
ALL_THREADS := $(shell echo $(THREADS)*$(CORES) | bc)
SIM_OPTIONS += --core-threads=$(THREADS) --pod-cores=$(CORES)
RISCV_COMPILE_FLAGS += -DTHREADS=$(ALL_THREADS)
RISCV_COMPILE_FLAGS += -DBARRIER
include $(DRV_DIR)/mk/riscv_common.mk
