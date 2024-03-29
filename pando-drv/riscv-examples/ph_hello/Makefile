# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
RISCV_CSOURCE := $(wildcard *.c)
SIM_OPTIONS += --verbose 0
include $(DRV_DIR)/mk/riscv_common.mk
