# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
APP_NAME := to_address
SIM_OPTIONS += --pxn-pods=2 --pod-cores=4 --core-threads=1
SIM_OPTIONS += --drvx-stack-in-l1sp
SIM_OPTIONS += --num-pxn=2
include $(DRV_DIR)/mk/application_common.mk
