# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)

RISCV_CSOURCE := $(wildcard *.c)

SIM_OPTIONS += --pod-cores=1 --core-threads=1
SIM_OPTIONS += --debug-syscalls

include $(DRV_DIR)/mk/riscv_common.mk
