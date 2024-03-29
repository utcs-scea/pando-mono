# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)

RISCV_CSOURCE := $(wildcard *.c)

STORE_ADDR ?= 0x000000004100101f

RISCV_COMPILE_FLAGS += -DSTORE_ADDR=$(STORE_ADDR)
RISCV_COMPILE_FLAGS += -DSTORE_VALUE=0xfe

SIM_OPTIONS = --core-threads=1 --pod-cores=1

include $(DRV_DIR)/mk/riscv_common.mk
