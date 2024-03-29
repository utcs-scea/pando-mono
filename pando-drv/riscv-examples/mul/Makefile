# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
RISCV_ASMSOURCE := $(wildcard *.S)
RISCV_PLATFORM_CRT := no # this tests defines its own _start
include $(DRV_DIR)/mk/riscv_common.mk
