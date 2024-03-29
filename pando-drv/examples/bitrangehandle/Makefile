# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
APP_NAME := bitrangehandle
include $(DRV_DIR)/mk/application_common.mk

SIM_OPTIONS += --core-threads=1 --pod-cores=1
