# SPDX-License-Identifier: MIT
# Copyright (c) 2024 University of Washington

# import parameters and APP_PATH
include parameters.mk
include app_path.mk

DRV_DIR := $(shell git rev-parse --show-toplevel)
vpath %.c    $(APP_PATH)
vpath %.cpp  $(APP_PATH)
vpath %.c    $(DRV_DIR)/examples/gups_multi_node
vpath %.cpp  $(DRV_DIR)/examples/gups_multi_node

$(APP_NAME).o: gups_multi_node.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

APP_EXE ?= $(APP_PATH)/$(test-name)/$(APP_NAME).so

SIM_OPTIONS += --num-pxn=1 --pxn-pod=1 --pod-cores=$(pod-cores) --core-threads=$(core-threads)
SIM_OPTIONS += --dram-access-time=$(dram-latency) --pxn-dram-banks=$(dram-banks)
SIM_ARGS += $(shell echo 128*1024*1024|bc) 1024

include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_common.mk


.DEFAULT_GOAL := help


.PHONY: debug
debug:
	@echo "APP_PATH: $(APP_PATH)"
	@echo "APP_NAME: $(APP_NAME)"
