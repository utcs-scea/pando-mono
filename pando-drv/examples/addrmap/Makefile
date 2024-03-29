# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
APP_NAME := addrmap
include $(DRV_DIR)/mk/application_common.mk

VADDR  = 0x0400000000000000 # word  0x0 on pxn 0, pod 0's l2sp
VADDR += 0x0000000000000010 # word 0x10 on pxn 0, pod 0, executing core's local l1sp
VADDR += 0x0000000000100010 # word 0x10 on pxn 0, pod 0,  core 0's l1sp

tests := $(foreach vaddr,$(VADDR),run-$(vaddr))

run-%: $(APP_NAME).so
	$(SST) $(DRV_DIR)/tests/$(SCRIPT) --  --pod-cores 2 --core-threads 1 $(APP_PATH)/$(APP_NAME).so $*

# redefine run target
run: $(tests)
	@echo Running tests
