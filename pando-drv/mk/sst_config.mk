# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

ifndef _SST_CONFIG_MK_
_SST_CONFIG_MK_ := 1
DRV_DIR ?= $(shell git rev-parse --show-toplevel)

# CHANGEME - set the path to your sst-core install
SST_CORE_INSTALL_DIR=$(DRV_ROOT)
SST          := $(SST_CORE_INSTALL_DIR)/bin/sst
SST_CONFIG   := $(SST_CORE_INSTALL_DIR)/bin/sst-config
SST_REGISTER := $(SST_CORE_INSTALL_DIR)/bin/sst-register

# CHANGEME - set the path to your sst-elements install
SST_ELEMENTS_INSTALL_DIR=$(DRV_ROOT)
SST_ELEMENTS_CXXFLAGS += -isystem $(SST_ELEMENTS_INSTALL_DIR)/include
SST_ELEMENTS_LDFLAGS  += -L$(SST_ELEMENTS_INSTALL_DIR)/lib -Wl,-rpath,$(SST_ELEMENTS_INSTALL_DIR)/lib
SST_ELEMENTS_LDFLAGS  += -L$(SST_ELEMENTS_INSTALL_DIR)/lib/sst-elements-library
SST_ELEMENTS_LDFLAGS += -Wl,-rpath,$(SST_ELEMENTS_INSTALL_DIR)/lib/sst-elements-library
SST_ELEMENTS_LDFLAGS += -lmemHierarchy
endif
