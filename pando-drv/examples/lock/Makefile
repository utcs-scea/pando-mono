# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
APP_NAME := lock
include $(DRV_DIR)/mk/application_common.mk

MEM_BACKENDS-yes += simple
MEM_BACKENDS-$(RAMULATOR) += ramulator
MEM_BACKENDS-$(DRAMSIM3) += dramsim3
MEM_BACKENDS := $(MEM_BACKENDS-yes)

tests := $(foreach membackend,$(MEM_BACKENDS),run-$(membackend))

all-SIM_OPTIONS += --pod-cores=4 --core-threads=2

ramulator-SIM_OPTIONS := --dram-backend-config $(RAMULATOR_TEST_CONFIG)
dramsim3-SIM_OPTIONS := --dram-backend-config $(DRAMSIM3_TEST_CONFIG)

run-%: $(APP_NAME).so
	$(eval backend-SIM_OPTIONS=$($*-SIM_OPTIONS))
	$(SST) $(DRV_DIR)/tests/$(SCRIPT) -- --dram-backend $* $(SIM_OPTIONS) $(backend-SIM_OPTIONS) $(APP_PATH)/$(APP_NAME).so

run: $(tests)
	@echo Running tests
