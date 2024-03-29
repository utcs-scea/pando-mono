# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

APP_NAME := gups
DRV_DIR := $(shell git rev-parse --show-toplevel)

TABLE_SIZE := 1048576
THREAD_UPDATES ?= 1024
SIM_ARGS += $(TABLE_SIZE) $(THREAD_UPDATES)
SIM_OPTIONS += --num-pxn=2 --pxn-pods=2

include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_common.mk
