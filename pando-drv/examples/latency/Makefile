# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
APP_NAME := latency
include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_common.mk

CORE   ?= 0
THREAD ?= 0
ADDR   ?= 0x0
N      ?= 1

SIM_OPTIONS += --core-stats --stats-load-level=1
SIM_ARGS += $(CORE) $(THREAD) $(ADDR) $(N)
