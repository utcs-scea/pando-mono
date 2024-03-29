# SPDX-License-Identifier: MIT
# Copyright (c) 2024 University of Washington

# import parameters and APP_PATH
include parameters.mk
include app_path.mk

DRV_DIR := $(shell git rev-parse --show-toplevel)

vpath %.c   $(APP_PATH)
vpath %.cpp $(APP_PATH)

APP_EXE ?= $(APP_PATH)/$(test-name)/$(APP_NAME).so

include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_common.mk


.DEFAULT_GOAL := help


.PHONY: debug
debug:
  @echo "APP_PATH: $(APP_PATH)"
  @echo "APP_NAME: $(APP_NAME)"
