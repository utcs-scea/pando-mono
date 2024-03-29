# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
RISCV_CXXSOURCE := $(wildcard *.cpp)
SIM_OPTIONS += --num-pxn=1 --pxn-pods=1 --pod-cores=1 --core-threads=2
include $(DRV_DIR)/mk/riscv_common.mk
