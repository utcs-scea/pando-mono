# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

DRV_DIR := $(shell git rev-parse --show-toplevel)

THREADS_PER_CORE := 16
CORES_PER_POD    := 8

# pando hammer source files
RISCV_CXXSOURCE += bfs.cpp

# command processor compile flags
COMMAND_PROCESSOR_COMPILE_FLAGS += -DCOMMAND_PROCESSOR
COMMAND_PROCESSOR_COMPILE_FLAGS += -DTHREADS_PER_CORE=$(THREADS_PER_CORE)
COMMAND_PROCESSOR_COMPILE_FLAGS += -I$(DRV_DIR)/riscv-examples/bfs/sparse_matrix_helpers
COMMAND_PROCESSOR_PLATFORM_LOADER := no

vpath %.cpp $(DRV_DIR)/riscv-examples/bfs/sparse_matrix_helpers
vpath %.c   $(DRV_DIR)/riscv-examples/bfs/sparse_matrix_helpers

# command processor source files
COMMAND_PROCESSOR_CXXSOURCE += bfs_cp.cpp
COMMAND_PROCESSOR_CXXSOURCE += breadth_first_search_graph.cpp
COMMAND_PROCESSOR_CXXSOURCE += transpose_graph.cpp
COMMAND_PROCESSOR_CXXSOURCE += read_graph.cpp
COMMAND_PROCESSOR_CSOURCE   += mmio.c

SIM_OPTIONS += --core-threads=$(THREADS_PER_CORE)
SIM_OPTIONS += --pod-cores=$(CORES_PER_POD)

include $(DRV_DIR)/mk/riscv_common.mk
include $(DRV_DIR)/mk/command_processor.mk

run-%: $(RISCV_TARGET) $(COMMAND_PROCESSOR_TARGET)
	$(eval graph=$(word 1, $(subst -, ,$*)))
	$(eval root=$(word 2, $(subst -, ,$*)))
	$(MAKE) -C $(DRV_DIR)/riscv-examples/bfs/sparse-inputs $(graph).mtx
	$(SST) $(SCRIPT) -- $(SIM_OPTIONS) $(RISCV_TARGET) \
	$(DRV_DIR)/riscv-examples/bfs/sparse-inputs/$(graph).mtx \
	$(root)

run: $(RISCV_TARGET) $(COMMAND_PROCESSOR_TARGET)
run: run-u12k16-0
	@echo Overloaded default run target
