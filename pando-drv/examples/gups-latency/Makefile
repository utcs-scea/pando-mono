# SPDX-License-Identifier: MIT
# Copyright (c) 2024 University of Washington
DRV_DIR = $(shell git rev-parse --show-toplevel)

.PHONY: all
all: generate

# call to generate a test name
test-name = pod-cores_$(1)__core-threads_$(2)__dram-latency_$(3)__dram-banks_$(4)

# call to get parameter from test name
get-pod-cores = $(lastword $(subst _, ,$(filter pod-cores_%,$(subst __, ,$(1)))))
get-core-threads = $(lastword $(subst _, ,$(filter core-threads_%,$(subst __, ,$(1)))))
get-dram-latency = $(lastword $(subst _, ,$(filter dram-latency_%,$(subst __, ,$(1)))))
get-dram-banks = $(lastword $(subst _, ,$(filter dram-banks_%,$(subst __, ,$(1)))))

# defines tests
TESTS =
include tests.mk

TESTS_DIRS = $(TESTS)

$(addsuffix /parameters.mk,$(TESTS_DIRS)): %/parameters.mk:
	@echo Creating $@
	@mkdir -p $(dir $@)
	@touch $@
	@echo test-name  = $* >> $@
	@echo pod-cores = $(call get-pod-cores,$*) >> $@
	@echo core-threads = $(call get-core-threads,$*) >> $@
	@echo dram-latency = $(call get-dram-latency,$*) >> $@
	@echo dram-banks = $(call get-dram-banks,$*) >> $@

include $(DRV_DIR)/mk/testbench_common.mk

# for regression
EXECUTION_DIRS := (addprefix $(APP_PATH)/,$(TESTS_DIRS))
