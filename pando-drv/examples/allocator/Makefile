# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

APP_NAME := allocator
DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
SIM_OPTIONS += --pod-cores=2 --core-threads=2
include $(DRV_DIR)/mk/application_common.mk
