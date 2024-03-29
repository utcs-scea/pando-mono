# SPDX-License-Identifier: MIT

DRV_DIR := $(shell git rev-parse --show-toplevel)
APP_NAME := IDM
APP_PATH := $(DRV_DIR)/examples/IDM

include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_common.mk

run: $(APP_NAME).so
	sst $(DRV_DIR)/tests/PANDOHammerDrvX-IDM.py -- $(APP_PATH)/$(APP_NAME).so
