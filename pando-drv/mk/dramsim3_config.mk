# SPDX-License-Identifier: MIT
# Copyright (c) 2024 University of Washington

ifndef _DRAMSIM3_CONFIG_MK_
_DRAMSIM3_CONFIG_MK_ := 1
DRAMSIM3_DIR ?= /root/sst-dramsim3-src
DRAMSIM3 := $(shell [ -d $(DRAMSIM3_DIR) ] && echo yes || echo no)
DRAMSIM3_TEST_CONFIG ?= $(DRAMSIM3_DIR)/configs/HBM2_4Gb_x128.ini
DRAMSIM3_CXXFLAGS += -isystem $(DRAMSIM3_DIR)/src
DRAMSIM3_LDFLAGS  +=
DRAMSIM3_LIBS     +=
endif
