# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
APP_NAME := globals
include $(DRV_DIR)/mk/application_common.mk


run: $(APP_NAME).so
	$(SST) $(DRV_DIR)/tests/$(SCRIPT) --  \
	--num-pxn 2 --pod-cores 1 --core-threads \
	1 $(APP_PATH)/$(APP_NAME).so
