# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

APP_NAME := runtime
DRV_DIR := $(shell git rev-parse --show-toplevel)
APP_PATH := $(DRV_DIR)/examples/super_simple_task_runtime

SIM_OPTIONS += --num-pxn=2 --with-command-processor=$(APP_PATH)/$(APP_NAME).so

include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_common.mk

$(APP_NAME).so: main.o
