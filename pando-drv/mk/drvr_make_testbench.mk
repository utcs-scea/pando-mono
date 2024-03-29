# SPDX-License-Identifier: MIT
# Copyright (c) 2024 University of Washington

DRV_DIR = $(shell git rev-parse --show-toplevel)
TESTBENCH  ?= unnamed_benchmark
TESTBENCH_PATH ?= $(DRV_DIR)/riscv-examples/$(TESTBENCH)
PARAMETERS ?= p0 p1 p2

.PHONY: all
all: $(TESTBENCH_PATH)

$(TESTBENCH_PATH): $(TESTBENCH_PATH)/Makefile
$(TESTBENCH_PATH): $(TESTBENCH_PATH)/template.mk
$(TESTBENCH_PATH): $(TESTBENCH_PATH)/app_path.mk
$(TESTBENCH_PATH): $(TESTBENCH_PATH)/tests.mk
$(TESTBENCH_PATH): $(TESTBENCH_PATH)/main.c
$(TESTBENCH_PATH): $(TESTBENCH_PATH)/.gitignore

$(TESTBENCH_PATH)/Makefile: $(DRV_DIR)/py/benchmark_makefile_gen.py
  mkdir -p $(dir $@)
  python3 $< $(PARAMETERS) > $@

$(TESTBENCH_PATH)/template.mk: $(DRV_DIR)/mk/drvr_testbench_template.mk
  mkdir -p $(dir $@)
  cp $< $@

$(TESTBENCH_PATH)/app_path.mk:
  mkdir -p $(dir $@)
  echo 'APP_NAME = $(TESTBENCH)' > $@
  echo 'APP_PATH = $$(DRV_DIR)/riscv-examples/$(TESTBENCH)' >> $@

$(TESTBENCH_PATH)/tests.mk: $(DRV_DIR)/py/benchmark_tests_file_gen.py
  mkdir -p $(dir $@)
  python3 $< $(PARAMETERS) > $@

$(TESTBENCH_PATH)/main.c:
  mkdir -p $(dir $@)
  @echo "// SPDX-License-Identifier: MIT" > $@
  @echo "// Copyright (c) 2024 University of Washington" >> $@
  @echo "int main(int argc, char** argv) {" >> $@
  @echo "    return 0;" >> $@
  @echo "}" >> $@

$(TESTBENCH_PATH)/.gitignore:
  mkdir -p $(dir $@)
  echo "*/" > $@
