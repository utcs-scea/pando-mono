# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
APP_NAME := commandprocessor
include $(DRV_DIR)/mk/application_common.mk

run: $(APP_NAME).so
	$(SST) $(DRV_DIR)/tests/$(SCRIPT) -- \
	--with-command-processor=$(APP_PATH)/$(APP_NAME).so \
	--pod-cores=8 \
	--core-threads=16 \
	$(APP_PATH)/$(APP_NAME).so
