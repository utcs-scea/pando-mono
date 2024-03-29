# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

APP_NAME := gups_multi_node
DRV_DIR := $(shell git rev-parse --show-toplevel)

TABLE_SIZE := 1048576
THREAD_UPDATES ?= 1024
SIM_ARGS += $(TABLE_SIZE) $(THREAD_UPDATES)
SIM_OPTIONS += --num-pxn=2 --pxn-pods=2

include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_common.mk
