# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington
DRV_DIR ?= $(shell git rev-parse --show-toplevel)
DRV_ROOT ?= /install
ifndef _DRV_CONFIG_MK
_DRV_CONFIG_MK := 1
include $(DRV_DIR)/mk/sst_config.mk
include $(DRV_DIR)/mk/boost_config.mk
include $(DRV_DIR)/mk/install_config.mk
include $(DRV_DIR)/mk/application_config.mk

ifndef NO_DRVR
  include $(DRV_DIR)/mk/ramulator_config.mk
  include $(DRV_DIR)/mk/dramsim3_config.mk
  include $(DRV_DIR)/mk/riscv_config.mk
endif

endif
