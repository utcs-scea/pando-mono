# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
RISCV_CSOURCE := $(wildcard *.c)
SIM_OPTIONS += --core-threads=1 --pod-cores=1

include $(DRV_DIR)/mk/riscv_common.mk
