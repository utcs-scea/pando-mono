# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

ifndef _RAMULATOR_CONFIG_MK_
_RAMULATOR_CONFIG_MK_ := 1
RAMULATOR_DIR ?= /root/sst-ramulator-src
RAMULATOR := $(shell [ -d $(RAMULATOR_DIR) ] && echo yes || echo no)
RAMULATOR_TEST_CONFIG ?= $(RAMULATOR_DIR)/configs/HBM-config.cfg
RAMULATOR_CXXFLAGS += -isystem $(RAMULATOR_DIR)/src
RAMULATOR_LDFLAGS  +=
RAMULATOR_LIBS     +=
endif
