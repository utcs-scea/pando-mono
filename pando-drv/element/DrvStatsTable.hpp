// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

// #define DEFINE_DRV_STAT(name, desc, units, load_level)
DEFINE_DRV_STAT(LOAD_LOCAL_L1SP, "Number of loads from local l1sp", "instructions", 1)
DEFINE_DRV_STAT(STORE_LOCAL_L1SP, "Number of stores to local l1sp", "instructions", 1)
DEFINE_DRV_STAT(ATOMIC_LOCAL_L1SP, "Number of atomics to local l1sp", "instructions", 1)
DEFINE_DRV_STAT(LOAD_REMOTE_L1SP, "Number of loads from remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(STORE_REMOTE_L1SP, "Number of stores to remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(ATOMIC_REMOTE_L1SP, "Number of atomics to remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(LOAD_L2SP, "Number of loads from remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(STORE_L2SP, "Number of stores to remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(ATOMIC_L2SP, "Number of atomics to remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(LOAD_DRAM, "Number of loads from remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(STORE_DRAM, "Number of stores to remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(ATOMIC_DRAM, "Number of atomics to remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(LOAD_REMOTE_PXN, "Number of loads from remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(STORE_REMOTE_PXN, "Number of stores to remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(ATOMIC_REMOTE_PXN, "Number of atomics to remote l1sp", "instructions", 1)
DEFINE_DRV_STAT(STALL_CYCLES, "Number of cycles stalled", "cycles", 1)
DEFINE_DRV_STAT(BUSY_CYCLES, "Number of cycles busy", "cycles", 1)
