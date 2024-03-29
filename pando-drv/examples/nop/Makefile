# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

APP_NAME := nop
DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_common.mk

SIM_ARGS += 100
SIM_OPTIONS += --num-pxn=1 --pxn-pods=1 --pod-cores=1 --core-threads=1
