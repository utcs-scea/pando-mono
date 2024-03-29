# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

ifndef _INSTALL_CONFIG_MK_
_INSTALL_CONFIG_MK_ := 1

DRV_DIR ?= $(shell git rev-parse --show-toplevel)
# CHANGEME - where do you want to install the libraries and
# header files for drv
DRV_INSTALL_DIR = $(DRV_ROOT)

DRV_BIN_DIR = $(DRV_INSTALL_DIR)/bin
DRV_INCLUDE_DIR = $(DRV_INSTALL_DIR)/include
DRV_LIB_DIR = $(DRV_INSTALL_DIR)/lib

$(DRV_BIN_DIR) $(DRV_INCLUDE_DIR) $(DRV_LIB_DIR):
	@mkdir -p $@

$(DRV_INSTALL_DIR): $(DRV_BIN_DIR) $(DRV_INCLUDE_DIR) $(DRV_LIB_DIR)
	@mkdir -p $@

.PHONY: $(DRV_INSTALL_DIR) $(DRV_BIN_DIR) $(DRV_INCLUDE_DIR) $(DRV_LIB_DIR)

endif
