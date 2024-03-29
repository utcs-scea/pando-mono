# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR ?= $(shell git rev-parse --show-toplevel)

all: install examples examples-run

EXAMPLES = $(wildcard $(DRV_DIR)/examples/*/)

.PHONY: all install install-element install-api install-interpreter install-py
.PHONY: clean  examples $(EXAMPLES)

install: install-api install-element install-interpreter install-py

install-api:
	$(MAKE) -C $(DRV_DIR)/api/ install

install-interpreter:
	$(MAKE) -C $(DRV_DIR)/interpreter/ install

install-element: install-api install-interpreter
	$(MAKE) -C $(DRV_DIR)/element/ install

install-py:
	$(MAKE) -C $(DRV_DIR)/py/ install

$(foreach example, $(EXAMPLES), $(example)-clean): %-clean:
	$(MAKE) -C $* clean

$(foreach example, $(EXAMPLES), $(example)-run): %-run:
	$(MAKE) -C $* clean
	$(MAKE) -C $* run

clean: #$(foreach example, $(EXAMPLES), $(example)-clean)
	$(MAKE) -C $(DRV_DIR)/element/ clean
	$(MAKE) -C $(DRV_DIR)/api/ clean
	$(MAKE) -C $(DRV_DIR)/interpreter/ clean
	$(MAKE) -C $(DRV_DIR)/py/ clean
	rm -rf install/

$(EXAMPLES): install-api install-element install-interpreter

examples: $(EXAMPLES)

examples-run: $(foreach example, $(EXAMPLES), $(example)-run)

$(EXAMPLES):
	$(MAKE) -C $@ all
