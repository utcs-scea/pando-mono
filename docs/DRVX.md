<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

# DRVX Simulator

DrvX is the PANDO team's fast-functional model for the PANDOHammer architecture.
It emulates the programming environment, including the PGAS, the threading model,
and the memory system of PANDOHammer hardware.
It runs the application natively.

## Simulator Run Time Speedup

There is a few ways you could speed up the simulator run time
if you are willing to tradeoff some accuracy or
only a certain part of the application is of interest.

### Enable DMA

Set `PANDO_RT_DRVX_ENABLE_DMA=ON`.
Instead of sending read / write request at a 8-byte granularity,
the packet will contain the whole object to be read / write.
This is kind of emulating the case where there is a DMA engine
but neglecting the message chunking and network contention part.

### Bypass SST Simulation

Set `PANDO_RT_DRVX_ENABLE_BYPASS=ON`.
Whenever a load or store occurs,
we don't call the *DrvAPI* functions anymore
to expose the memory operations to the SST simulator.
Instead, use *DrvAPIAddressToNative* to return the native pointer
which the memory backend of DrvX is memory mapped to
and directly manipulate that pointer.

1. Add `#include <pando-rt/drv_info.hpp>` in the file.
2. Add `PANDO_DRV_SET_BYPASS_FLAG();` in front of the code you wish to skip SST simulation
3. Add `PANDO_DRV_CLEAR_BYPASS_FLAG();` after the code to enable SST simulation

**Note:** This only works in a single thread simulation setting.
There is no guarantee this will still work when ported to multiple ranks.
