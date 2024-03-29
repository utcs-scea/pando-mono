# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
APP_NAME := atomic
include $(DRV_DIR)/mk/application_common.mk

N      ?= 1000000
TARGET ?= DRAM
MEMOP  ?= LOAD
DRAM_ACCESS_TIME ?= 13ns
MAX_IDLE ?= 1

THREADS ?= 16
CORES   ?= 8
PODS    ?= 2
PXNS    ?= 2

CXXFLAGS += -DNATOMICS=$(N) -DTARGET_$(TARGET) -DMEMOP_$(MEMOP)
SIM_OPTIONS += --core-threads=$(THREADS) --pod-cores=$(CORES) --pxn-pods=$(PODS) --num-pxn=$(PXNS)
SIM_OPTIONS += --dram-access-time=$(DRAM_ACCESS_TIME)
SIM_OPTIONS += --core-max-idle=$(MAX_IDLE)

MEM_BACKENDS-yes += simple
#MEM_BACKENDS-$(RAMULATOR) += ramulator
MEM_BACKENDS-$(DRAMSIM3) += dramsim3
MEM_BACKENDS := $(MEM_BACKENDS-yes)

tests := $(foreach membackend,$(MEM_BACKENDS),run-$(membackend))

ramulator-SIM_OPTIONS := --dram-backend-config $(RAMULATOR_TEST_CONFIG)
dramsim3-SIM_OPTIONS := --dram-backend-config $(DRAMSIM3_TEST_CONFIG)

run-%: $(APP_NAME).so
	$(eval backend-SIM_OPTIONS=$($*-SIM_OPTIONS))
	@echo N=$(N)
	@echo TARGET=$(TARGET)
	@echo MEMOP=$(LOAD)
	@echo DRAM_ACCESS_TIME=$(DRAM_ACCESS_TIME)
	@echo MAX_IDLE=$(MAX_IDLE)
	$(SST) -n $(SIM_THREADS) $(DRV_DIR)/tests/$(SCRIPT) -- --dram-backend $* $(SIM_OPTIONS) $(backend-SIM_OPTIONS) $(APP_PATH)/$(APP_NAME).so

run: $(tests)
	@echo Running tests
