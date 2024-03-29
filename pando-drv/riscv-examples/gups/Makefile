# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
THREAD_UPDATES ?= 1024
TABLE_SIZE ?= $(shell echo 128*1024*1024 | bc)
RISCV_CSOURCE := $(wildcard *.c)
RISCV_COMPILE_FLAGS += -DDRAM_BASE=$(shell echo 2^47 | bc) -DDRAM_SIZE=$(shell echo 2^30 | bc)
RISCV_COMPILE_FLAGS += -DTHREAD_UPDATES=$(THREAD_UPDATES) -DTABLE_SIZE=$(TABLE_SIZE)
SIM_OPTIONS += --pxn-pods=2
include $(DRV_DIR)/mk/riscv_common.mk
