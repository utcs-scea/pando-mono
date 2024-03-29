# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)

RISCV_ASMSOURCE := $(wildcard *.S)
RISCV_CSOURCE := $(wildcard *.c)
RISCV_PLATFORM_CRT := no # this tests defines its own _start

SIM_OPTIONS += --pod-cores=1 --core-threads=1 --verbose=1
include $(DRV_DIR)/mk/riscv_common.mk
