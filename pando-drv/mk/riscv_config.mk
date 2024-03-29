RISCV_INSTALL_DIR=$(DRV_ROOT)
# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

ifndef _RISCV_CONFIG_MK_
_RISCV_CONFIG_MK_ := 1

RISCV_ARCH ?= unknown-elfpandodrvsim
RISCV_ARCH_INSTALL_DIR=$(RISCV_INSTALL_DIR)/riscv64-$(RISCV_ARCH)

endif
