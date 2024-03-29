# SPDX-License-Identifier: MIT
# Copyright (c) 2024 University of Washington

# import parameters and APP_PATH
include parameters.mk
include app_path.mk

DRV_DIR := $(shell git rev-parse --show-toplevel)
TEST_DIR :=$(APP_PATH)/$(test-name)

vpath %.c   $(APP_PATH)
vpath %.cpp $(APP_PATH)

RISCV_CSOURCE   := main.c
RISCV_CXXSOURCE :=
RISCV_ASMSOURCE :=

include $(DRV_DIR)/mk/riscv_common.mk


.DEFAULT_GOAL := help


.PHONY: debug
debug:
	@echo "APP_PATH: $(APP_PATH)"
	@echo "APP_NAME: $(APP_NAME)"
