# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR ?= $(shell git rev-parse --show-toplevel)
include $(DRV_DIR)/mk/config.mk
include $(DRV_DIR)/mk/application_config.mk

drvx_compile_flags := $(DRV_BIN_DIR)/drv-x-cxxflags
drvx_linker_flags := $(DRV_BIN_DIR)/drv-x-ldflags

targets := $(drvx_compile_flags) $(drvx_linker_flags)

.PHONY: install clean
.PHONY: $(drvx_compile_flags)

$(drvx_compile_flags): SCRIPT_FLAGS := $(APP_CXXFLAGS)
$(drvx_linker_flags):  SCRIPT_FLAGS := $(APP_LDFLAGS)

$(targets): $(DRV_DIR)/py/make-drv-compile-flags.py
	@mkdir -p $(@D)
	python3 $< $(SCRIPT_FLAGS) > $@
	chmod +x $@

install: $(targets)

clean:
	@echo "Nothing to clean"
